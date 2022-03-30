#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "common_macros.h"
#include "BatteryModel.h"
#include "f29BmsConfig.h"
#include "TempModel.h"
#include "BmsSimClient.h"
#include "BmsSimConfig.h"
#include "Mailbox.h"
#include "PbSockets.h"
#include "BmsSim.pb.h"

static int socket_desc;

// output state
static BmsOut BmsOut_state = BmsOut_init_zero;
// output state of cell info, only drain_status should be used in this
static CellList out_CellList = CellList_init_zero;

// input state
static BmsIn BmsIn_state = BmsIn_init_zero;
static Cell in_CellList[NUM_SERIES_CELLS];
static Therm in_ThermList[NUM_THERMISTOR];

// flags for avoiding excessive I/O
static bool diff_BmsIn_state = false;
static bool diff_in_CellList = false;
static bool diff_in_ThermList = false;
static bool diff_BmsOut_state = false;
static bool diff_out_CellList = false;


int BmsSimClient_init(void)
{
    
	struct sockaddr_in server;
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("f29bms: Could not create socket");
        return -1;
	}
		
	server.sin_addr.s_addr = inet_addr(BMS_IP);
	server.sin_family = AF_INET;
	server.sin_port = htons(BMS_PORT);

	int err_code = connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0;
	if (err_code != 0)
	{
		printf("f29bms: Could not connect to server, error code: %d\n", err_code);
		exit(-1);
	}
	
	puts("f29bms: Connected to sim harness.\n");

	Mailbox_init(socket_desc);

	out_CellList.cells_count = 0;
	BmsIn_state.charger_available = 0;

	return 0;
}

static void message_handler(BmsData* data)
{
	switch (data->which_data)
	{
		case BmsData_bmsIn_tag:
			// update the input data state
			BmsIn_state = data->data.bmsIn;
			break;
		
		case BmsData_cell_list_tag:
			{
				CellList cells = data->data.cell_list;
				for (int i = 0; i < cells.cells_count; i++)
				{
					Cell new_cell = cells.cells[i];
					in_CellList[SAT(new_cell.index, 0, NUM_SERIES_CELLS-1)] = new_cell;
				}
			}
			break;
		
		case BmsData_therm_list_tag:
			{
				ThermList therms = data->data.therm_list;
				for (int i = 0; i < therms.therms_count; i++)
				{
					Therm new_therm = therms.therms[i];
					in_ThermList[new_therm.index] = new_therm;
				}
			}
			break;

		default:
			printf("f29bms: Unknown input type detected\n");
			break;
	}
}

BmsOut* BmsSimClient_get_outputs(void)
{
	// trigger a send next tick
	diff_BmsOut_state = true;

	// drivers must be careful with this
	return &BmsOut_state;
}

const BmsIn* BmsSimClient_get_inputs(void)
{
	return &BmsIn_state;
}

Cell* BmsSimClient_get_cells_in(void)
{
	return in_CellList;
}

Therm* BmsSimClient_get_therms_in(void)
{
	return in_ThermList;
}

void BmsSimClient_set_cell_data(int index, bool drain_state)
{
	if (out_CellList.cells_count < NUM_SERIES_CELLS)
	{
		diff_out_CellList = true;
		
		Cell new_cell = Cell_init_zero;
		new_cell.is_draining = drain_state;
		new_cell.index = index;
		out_CellList.cells[out_CellList.cells_count] = new_cell;
		out_CellList.cells_count++;
	}
}

void BmsSimClient_io(void)
{
	static int iteration = 0;

	if (iteration == 0)
	{
		Mailbox_receive();
		BmsData data = BmsData_init_zero;
		while(!Mailbox_read_from_inbox(&data))
		{
			message_handler(&data);	
		}
	}

	iteration = (iteration + 1) % INPUT_PERIOD_MS;

	// send most recent sim data
	BmsData bmsdata = BmsData_init_zero;
	bmsdata.which_data = BmsData_bmsOut_tag;
	bmsdata.data.bmsOut = BmsOut_state;
	BmsOut_state.watchdog_pet = false;

	
	Mailbox_add_to_outbox(&bmsdata);

	if (out_CellList.cells_count > 0)
	{
		BmsData celldata = BmsData_init_zero;
		celldata.which_data = BmsData_cell_list_tag;
		celldata.data.cell_list = out_CellList;
		Mailbox_add_to_outbox(&celldata);
	}
	
	Mailbox_send();

	bool diff_BmsIn_state = false;
	bool diff_in_CellList = false;
	bool diff_in_ThermList = false;
	bool diff_BmsOut_state = false;
	out_CellList.cells_count = 0;
}

int BmsSimClient_send_CAN(int id, uint64_t data)
{
	BmsData msg;
	msg.which_data = BmsData_can_tag;
	msg.data.can.id = id;
	msg.data.can.data = (int64_t) data;	
	return Mailbox_add_to_outbox(&msg);
}

void BmsSimClient_close(void)
{
	printf("f29bms: Closing socket\n");
	int closecode;
	if ((closecode = close(socket_desc)) != 0)
	{
		printf("f29bms: Problem closing BmsSimClient Socket: %d\n", closecode);
	}
}