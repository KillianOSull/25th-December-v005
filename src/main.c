#include <zephyr/types.h> //includes a type of library
#include <stddef.h>	  //includes a type of library
#include <string.h>	  //includes a type of library
#include <errno.h> 	  //includes a type of library
#include <sys/printk.h>	  //includes a type of library
#include <sys/byteorder.h>//includes a type of library
#include <zephyr.h>	  //includes a type of library

#include <settings/settings.h>	//includes a type of library

#include <bluetooth/bluetooth.h>//includes a type of library
#include <bluetooth/hci.h>	//includes a type of library
#include <bluetooth/conn.h> 	//includes a type of library
#include <bluetooth/uuid.h>	//includes a type of library
#include <bluetooth/gatt.h>	//includes a type of library
#include <device.h>		//includes a type of library
#include <drivers/sensor.h>	//includes a type of library

#include <stdio.h>		//includes a type of library
		

#include "matrix.h" 		//includes matrix.h file in the directory assignment
#include "buttons.h"		//includes a buttons.h file in the directory assignment

#define BT_UUID_LED_SERVICE_VAL BT_UUID_128_ENCODE(1, 2, 3, 4, (uint64_t)0x3000)		//defines the bluetooth identifier
#define BT_UUID_LED_ID BT_UUID_128_ENCODE(1, 2, 3, 4, (uint64_t)0x3001)				//defines the bluetooth identifier
#define BT_UUID_BUTTON_SERVICE_VAL BT_UUID_128_ENCODE(1, 2, 3, 4, (uint64_t)0x2000)		//defines the bluetooth identifier
#define BT_UUID_BUTTON_A_ID  	   BT_UUID_128_ENCODE(1, 2, 3, 4, (uint64_t)0x2001)		//defines the bluetooth identifier
#define BT_UUID_BUTTON_B_ID  	   BT_UUID_128_ENCODE(1, 2, 3, 4, (uint64_t)24)			//defines the bluetooth identifier

static struct bt_uuid_128 my_led_uuid = BT_UUID_INIT_128( BT_UUID_LED_SERVICE_VAL);		// the 128 bit UUID for this gatt value
static struct bt_uuid_128 led1_id = BT_UUID_INIT_128(BT_UUID_LED_ID); 		   		// the 128 bit UUID for this gatt value

static struct bt_uuid_128 my_button_uuid = BT_UUID_INIT_128( BT_UUID_BUTTON_SERVICE_VAL);	// the 128 bit UUID for this gatt value
static struct bt_uuid_128 button_a_id=BT_UUID_INIT_128(BT_UUID_BUTTON_A_ID); 			// the 128 bit UUID for this gatt value
static struct bt_uuid_128 button_b_id=BT_UUID_INIT_128(BT_UUID_BUTTON_B_ID); 			// the 128 bit UUID for this gatt value

uint32_t led1_value;		// the gatt characateristic value that is being shared over BLE

uint32_t button_a;		// the gatt characateristic value that is being shared over BLE 
uint32_t button_b;		// the gatt characateristic value that is being shared over BLE 
uint32_t button_a_state=0;	// the gatt characateristic value that is being shared over BLE  
uint32_t button_b_state=0;	// the gatt characateristic value that is being shared over BLE 

static ssize_t read_led(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset);
//callback that is activated when the characteristic is read by central
static ssize_t read_button_a(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset);
//callback that is activated when the characteristic is read by central
static ssize_t read_button_b(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset);
//callback that is activated when the characteristic is read by central

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),	/* specify BLE advertising flags = discoverable, BR/EDR not supported (BLE only) */
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LED_SERVICE_VAL 		/* A 128 Service UUID for the our custom service follows */),
};

struct bt_conn *active_conn=NULL; // use this to maintain a reference to the connection with the central device (if any)

static ssize_t read_button_a(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset)
{
	printf("Got a read on button A state\n");	//prints on commandline when button A is pressed
	const char *value = (const char *)&button_a_state;// point at the value in memory
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(button_a_state));
}
static ssize_t read_button_b(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset)
{
	printf("Got a read on button B state\n");	//prints on commandline when button B is pressed
	const char *value = (const char *)&button_b_state;// point at the value in memory
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(button_b_state));
}

BT_GATT_SERVICE_DEFINE(my_button_svc,				//creates a new service for button
	BT_GATT_PRIMARY_SERVICE(&my_button_uuid),
		BT_GATT_CHARACTERISTIC(&button_a_id.uuid,		
		BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		BT_GATT_PERM_READ,
		read_button_a, NULL, &button_a_state),		//enables the read feature
		
		BT_GATT_CHARACTERISTIC(&button_b_id.uuid,		
		BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		BT_GATT_PERM_READ,
		read_button_b, NULL, &button_b_state)		//enables the read feature
		
);

static ssize_t read_led(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset)
{
	printf("Got a read to led state %p\n",attr); 	//prints when reading the led state
	matrix_begin();					//function to initilise all LED's on microbit
	// Could use 'const char *value =  attr->user_data' also here if there is the char value is being maintained with the BLE STACK
	const char *value = (const char *)&led1_value; // point at the value in memory
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(led1_value)); // pass the value back up through the BLE stack
	
}


BT_GATT_SERVICE_DEFINE(my_led_svc,			//creates a new service for LED
	BT_GATT_PRIMARY_SERVICE(&my_led_uuid),
		BT_GATT_CHARACTERISTIC(&led1_id.uuid,		
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_led, NULL, &led1_value),		//enables the read and write feature
);

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printf("Connection failed (err 0x%02x)\n", err); //prints if the connection fails
	} else {
		printf("Connected\n");
		active_conn = conn;				//puts conn into active_conn when connected
	}
}
// Callback that is activated when a connection with a central device is taken down
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);
	active_conn = NULL;					//puts NULL into active_conn when disconnected
}
// structure used to pass connection callback handlers to the BLE stack
static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};
// This is called when the BLE stack has finished initializing
static void bt_ready(void)
{
	int err; 				//local veriable called err
	printf("Bluetooth initialized\n"); 	//prints when br_ready function is called
	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printf("Advertising failed to start (err %d)\n", err);
		return;
	} //prints if there a fault for advertising
	printf("Advertising successfully started\n");
}

void main(void)				//main
{	
	int lock_code[100][32] = 	{ 0b0,0b0,0b1,0b0,0b1,0b1,0b0,0b1,0b0,0b0,0b1,0b0,0b1,0b0,0b0,0b1,0b0,0b1,0b0,0b0,0b1,0b0,0b1,0b1,0b1,0b0,0b1,0b0,0b0,0b1,0b0,0b1 };
	int unlock_code[32] = 		{ 0b1,0b1,0b0,0b1,0b0,0b0,0b1,0b0,0b0,0b0,0b1,0b1,0b0,0b1,0b0,0b0,0b1,0b1,0b0,0b1,0b1,0b0,0b1,0b1,0b1,0b0,0b0,0b1,0b1,0b0,0b0,0b1 };
	int op_xor[9] = { 0,0,0,0,0,0,0,0,0 };
	int match = 0;
	int err;			//new variable err
	int counter = 0;
	uint8_t rows = 1;		//initilsing rows for led matrix 
	uint8_t cols = 1;		//initilsing cols for led matrix
		//calling lsm303_ll_begin function in lsm303_ll.c
	err = buttons_begin();	//calling buttons begin function in buttons.c
	if (err < 0)
	{
		printf("\nError initializing buttons.  Error code = %d\n",err);	
	 while(1);
	} //error checking for buttons
	err = matrix_begin();
	if (err < 0)
	{
		printf("\nError initializing buttons.  Error code = %d\n",err);	
	 while(1);
	}//error checking for led matrix
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}	//error checking for bluetooth
	bt_ready(); // This function starts advertising
	bt_conn_cb_register(&conn_callbacks);
	printf("Zephyr Microbit V2 minimal BLE example! %s\n", CONFIG_BOARD);			
	while (1) {
	
		//k_sleep(K_SECONDS(0.2));				//sleeps for 1 second
		matrix_all_off();
			if(get_buttonA() == 0){
					
				printf("\nButton A pressed!\n");	// prints when button A has been pressed
				counter++;
				printf("Counter value: %d\n", counter);
				printf("UNLOCK CODE BEFORE ALTERING:		");
				
				for(int i = 0; i < 32; i++){
					printf("%d", unlock_code[i]);
				}
				
				//logic for rolling code, elements of interger array will be xored together to get a new code.
           			
           			int temp_unlock_code[32] = { 0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0 };
           			//int compare_unlock_code[32] = { 0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0,0b0 };
           			
           			for(int i = 0; i < 32; i++){
           				temp_unlock_code[i] = unlock_code[i];
           				//compare_unlock_code[i] = unlock_code[i];
           			}printf("\n");
           			
           			//first copy val
           			temp_unlock_code[0] = temp_unlock_code[5];
           			//xor0
           			if (temp_unlock_code[3] == 0b0 && temp_unlock_code[4] == 0b0) {op_xor[0] = 0;}
				if (temp_unlock_code[3] == 0b0 && temp_unlock_code[4] == 0b1) {op_xor[0] = 1;}
				if (temp_unlock_code[3] == 0b1 && temp_unlock_code[4] == 0b0) {op_xor[0] = 1;}
				if (temp_unlock_code[3] == 0b1 && temp_unlock_code[4] == 0b1) {op_xor[0] = 0;}
			    	//xor1
			    	if (temp_unlock_code[7] == 0b0 && op_xor[0] == 0) {temp_unlock_code[6] = 0b0;} //op of xor 1
				if (temp_unlock_code[7] == 0b0 && op_xor[0] == 1) {temp_unlock_code[6] = 0b1;}
				if (temp_unlock_code[7] == 0b1 && op_xor[0] == 0) {temp_unlock_code[6] = 0b1;}
				if (temp_unlock_code[7] == 0b1 && op_xor[0] == 1) {temp_unlock_code[6] = 0b0;}
           			//second copy val
           			temp_unlock_code[11] = temp_unlock_code[8];
           			//xor3
           			if (temp_unlock_code[12] == 0b0 && temp_unlock_code[13] == 0b0) {op_xor[3] = 0;}
				if (temp_unlock_code[12] == 0b0 && temp_unlock_code[13] == 0b1) {op_xor[3] = 1;}
				if (temp_unlock_code[12] == 0b1 && temp_unlock_code[13] == 0b0) {op_xor[3] = 1;}
				if (temp_unlock_code[12] == 0b1 && temp_unlock_code[13] == 0b1) {op_xor[3] = 0;}
			    	//xor2
           			if (temp_unlock_code[10] == 0b0 && op_xor[3] == 0) {temp_unlock_code[2] = 0b0;} //op of xor 2
				if (temp_unlock_code[10] == 0b0 && op_xor[3] == 1) {temp_unlock_code[2] = 0b1;}
				if (temp_unlock_code[10] == 0b1 && op_xor[3] == 0) {temp_unlock_code[2] = 0b1;}
				if (temp_unlock_code[10] == 0b1 && op_xor[3] == 1) {temp_unlock_code[2] = 0b0;}
           			//xor4
           			if (temp_unlock_code[15] == 0b0 && op_xor[3] == 0) {temp_unlock_code[1] = 0b0;} //op of xor 4
				if (temp_unlock_code[15] == 0b0 && op_xor[3] == 1) {temp_unlock_code[1] = 0b1;}
				if (temp_unlock_code[15] == 0b1 && op_xor[3] == 0) {temp_unlock_code[1] = 0b1;}
				if (temp_unlock_code[15] == 0b1 && op_xor[3] == 1) {temp_unlock_code[1] = 0b0;}
				//xor5
				if (temp_unlock_code[16] == 0b0 && temp_unlock_code[17] == 0b0) {	temp_unlock_code[14] = 0; 
												 	temp_unlock_code[18] = 0;}
												 	
				if (temp_unlock_code[16] == 0b0 && temp_unlock_code[17] == 0b1) {	temp_unlock_code[14] = 1;
													temp_unlock_code[18] = 1;}
													
				if (temp_unlock_code[16] == 0b1 && temp_unlock_code[17] == 0b0) {	temp_unlock_code[14] = 1; 
													temp_unlock_code[18] = 1;}
													
				if (temp_unlock_code[16] == 0b1 && temp_unlock_code[17] == 0b1) {	temp_unlock_code[14] = 0; 
													temp_unlock_code[18] = 0;}
				//xor6
				if (temp_unlock_code[20] == 0b0 && temp_unlock_code[21] == 0b0) {temp_unlock_code[19] = 0b0;}
				if (temp_unlock_code[20] == 0b0 && temp_unlock_code[21] == 0b1) {temp_unlock_code[19] = 0b1;}
				if (temp_unlock_code[20] == 0b1 && temp_unlock_code[21] == 0b0) {temp_unlock_code[19] = 0b1;}
				if (temp_unlock_code[20] == 0b1 && temp_unlock_code[21] == 0b1) {temp_unlock_code[19] = 0b0;}
				//third copy val
				temp_unlock_code[23] = temp_unlock_code[31];
				//xor7
				if (temp_unlock_code[24] == 0b0 && temp_unlock_code[25] == 0b0) {temp_unlock_code[27] = 0b0;} //op of xor7
				if (temp_unlock_code[24] == 0b0 && temp_unlock_code[25] == 0b1) {temp_unlock_code[27] = 0b1;}
				if (temp_unlock_code[24] == 0b1 && temp_unlock_code[25] == 0b0) {temp_unlock_code[27] = 0b1;}
				if (temp_unlock_code[24] == 0b1 && temp_unlock_code[25] == 0b1) {temp_unlock_code[27] = 0b0;}
				//xor8
				if (temp_unlock_code[28] == 0b0 && temp_unlock_code[29] == 0b0) {temp_unlock_code[22] = 0b0;} //op of xor8
				if (temp_unlock_code[28] == 0b0 && temp_unlock_code[29] == 0b1) {temp_unlock_code[22] = 0b1;}
				if (temp_unlock_code[28] == 0b1 && temp_unlock_code[29] == 0b0) {temp_unlock_code[22] = 0b1;}
				if (temp_unlock_code[28] == 0b1 && temp_unlock_code[29] == 0b1) {temp_unlock_code[22] = 0b0;}
				
				//putting altered code back into unlock code
				for(int i = 0; i < 32; i++){
           				unlock_code[i] = temp_unlock_code[i]; //transmitt code
           			}
           		
           			
/*

           			if(unlock_code[0] == compare_unlock_code[0] && unlock_code[1] == compare_unlock_code[1] && unlock_code[2] == compare_unlock_code[2] && unlock_code[3] == compare_unlock_code[3] && unlock_code[4] == compare_unlock_code[4] && unlock_code[5] == compare_unlock_code[5] && unlock_code[6] == compare_unlock_code[6] && unlock_code[7] == compare_unlock_code[7] && unlock_code[8] == compare_unlock_code[8] && unlock_code[9] == compare_unlock_code[9] && unlock_code[10] == compare_unlock_code[10] && unlock_code[11] == compare_unlock_code[11] && unlock_code[12] == compare_unlock_code[12] && unlock_code[13] == compare_unlock_code[13] && unlock_code[14] == compare_unlock_code[14] && unlock_code[15] == compare_unlock_code[15] && unlock_code[16] == compare_unlock_code[16] && unlock_code[17] == compare_unlock_code[17] && unlock_code[18] == compare_unlock_code[18] && unlock_code[19] == compare_unlock_code[19] && unlock_code[20] == compare_unlock_code[20] && unlock_code[21] == compare_unlock_code[21] && unlock_code[22] == compare_unlock_code[22] && unlock_code[23] == compare_unlock_code[23] && unlock_code[24] == compare_unlock_code[24] && unlock_code[25] == compare_unlock_code[25] && unlock_code[26] == compare_unlock_code[26] && unlock_code[27] == compare_unlock_code[27] && unlock_code[28] == compare_unlock_code[28] && unlock_code[29] == compare_unlock_code[29] && unlock_code[30] == compare_unlock_code[30] && unlock_code[31] == compare_unlock_code[31]){
           			match++;
           			printf("THE CODE HAS COPIED ITSELF AT ITTERATION: %d", itteration);
           			}   
           			
*/           			
           			
           			printf("UNLOCK CODE AFTER ALTERING:		");
           			for(int i = 0; i < 32; i++){
					printf("%d", unlock_code[i]);
					temp_unlock_code[i] = 0b0; //initilised temp_unlockcode back to 0b0
				}printf("\n");
				


           			//https://numbergenerator.org/randomnumbergenerator/0-31#!numbers=32&low=0&high=31&unique=true&csv=&oddeven=&oddqty=0&sorted=false&addfilters=
           			//temp_unlock_code - 1 10 29 28 6 30 3 16 13 21 15 12 31 9 17 11 26 18 20 22 2 23 0 24 14 4 7 8 19 27 25 5
           			//     unlock_code - 7 11 2 0 22 17 29 28 16 13 21 15 5 24 9 18 1 31 26 3 19 20 27 30 12 4 14 6 8 23 10 25
           			//uses above configureation to get random numbers beeen 0 - 31.
           			
           			temp_unlock_code[1] = unlock_code[7];
           			temp_unlock_code[10] = unlock_code[11];
           			temp_unlock_code[29] = unlock_code[2];
           			temp_unlock_code[28] = unlock_code[0];
           			temp_unlock_code[6] = unlock_code[22];
           			temp_unlock_code[30] = unlock_code[17];
           			temp_unlock_code[3] = unlock_code[29];
           			temp_unlock_code[16] = unlock_code[28];
           			temp_unlock_code[13] = unlock_code[16];
           			temp_unlock_code[21] = unlock_code[13];
           			temp_unlock_code[15] = unlock_code[21];
           			temp_unlock_code[12] = unlock_code[15];
           			temp_unlock_code[31] = unlock_code[5];
           			temp_unlock_code[9] = unlock_code[24];
           			temp_unlock_code[17] = unlock_code[9];
           			temp_unlock_code[11] = unlock_code[18];
           			temp_unlock_code[26] = unlock_code[1];
           			temp_unlock_code[18] = unlock_code[31];
           			temp_unlock_code[20] = unlock_code[26];
           			temp_unlock_code[22] = unlock_code[3];
           			temp_unlock_code[2] = unlock_code[19];
           			temp_unlock_code[23] = unlock_code[20];
           			temp_unlock_code[0] = unlock_code[27];
           			temp_unlock_code[24] = unlock_code[30];
           			temp_unlock_code[14] = unlock_code[12];
           			temp_unlock_code[4] = unlock_code[4];
           			temp_unlock_code[7] = unlock_code[14];
           			temp_unlock_code[8] = unlock_code[6];
           			temp_unlock_code[19] = unlock_code[8];
           			temp_unlock_code[27] = unlock_code[23];
           			temp_unlock_code[25] = unlock_code[10];
           			temp_unlock_code[5] = unlock_code[25];
           			
           			for(int i = 0; i < 32; i++){
           				unlock_code[i] = temp_unlock_code[i];
           			}
           			printf("UNLOCK CODE AFTER ALTERING, SHIFTED:	");
           			for(int i = 0; i < 32; i++){
					printf("%d", unlock_code[i]);
					temp_unlock_code[i] = 0b0; //initilised temp_unlockcode back to 0b0
				}printf("\n");
				
			
			
			//printf("match value: %d\n",match);
			if(match != 0){
				printf("THERES BEEN A MATCH");	
			}
				//displaying led pattern
				rows =  0b11111;		// sets the rows to all on		
		   		cols =  0b10000; 		// sets the cols to all off
		   		matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
		   		k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b11111;		// sets the rows to all on		
           			cols =  0b01000; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b11111;		// sets the rows to all on		
           			cols =  0b00100; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b11111;		// sets the rows to all on		
           			cols =  0b00010; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b11111;		// sets the rows to all on		
           			cols =  0b00001; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b11111;		// sets the rows to all on		
           			cols =  0b00010; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b11111;		// sets the rows to all on		
           			cols =  0b00100; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b11111;		// sets the rows to all on		
           			cols =  0b01000; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b11111;		// sets the rows to all on		
           			cols =  0b10000; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b11111;		// sets the rows to all on		
           			cols =  0b01000; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b11111;		// sets the rows to all on		
           			cols =  0b00100; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b11111;		// sets the rows to all on		
           			cols =  0b00010; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b11111;		// sets the rows to all on		
           			cols =  0b00001; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b11111;		// sets the rows to all on		
           			cols =  0b00010; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b11111;		// sets the rows to all on		
           			cols =  0b00100; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b11111;		// sets the rows to all on		
           			cols =  0b01000; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b11111;		// sets the rows to all on		
           			cols =  0b10000; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));

           			matrix_all_off();
           			k_sleep(K_SECONDS(1));
           			
		}
			
			
		
			if(get_buttonB() == 0)			
			{
				counter = 0;
				printf("\nButton B pressed!\n");	//prints when button B has been pressed
				printf("Counter value: %d\n", counter);
				printf("LOCK CODE:\n");
				
				for(int i = 0; i < 32; i++){
					printf("%d", lock_code[i]);
				}
				rows =  0b10000;		// sets the rows to all on		
           			cols =  0b11111; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b01000;		// sets the rows to all on		
           			cols =  0b11111; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b00100;		// sets the rows to all on		
           			cols =  0b11111; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b00010;		// sets the rows to all on		
           			cols =  0b11111; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b00001;		// sets the rows to all on		
           			cols =  0b11111; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b00010;		// sets the rows to all on		
           			cols =  0b11111; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b00100;		// sets the rows to all on		
           			cols =  0b11111; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b01000;		// sets the rows to all on		
           			cols =  0b11111; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b10000;		// sets the rows to all on		
           			cols =  0b11111; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b01000;		// sets the rows to all on		
           			cols =  0b11111; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b00100;		// sets the rows to all on		
           			cols =  0b11111; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b00010;		// sets the rows to all on		
           			cols =  0b11111; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b00001;		// sets the rows to all on		
           			cols =  0b11111; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b00010;		// sets the rows to all on		
           			cols =  0b11111; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b00100;		// sets the rows to all on		
           			cols =  0b11111; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b01000;		// sets the rows to all on		
           			cols =  0b11111; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));
           			
           			rows =  0b10000;		// sets the rows to all on		
           			cols =  0b11111; 		// sets the cols to all off
           			matrix_put_pattern(rows, ~cols);// put the pattern on the matrix (tilda, ~ inverts cols)
           			k_sleep(K_SECONDS(0.05));

           			matrix_all_off();
           			k_sleep(K_SECONDS(1));
			}
		if (active_conn)				//if there is an active connection, notify the stepcount, button A and Button B
		{
		bt_gatt_notify(active_conn,&my_button_svc.attrs[2], &button_a_state,sizeof(button_a_state));
		bt_gatt_notify(active_conn,&my_button_svc.attrs[3], &button_b_state,sizeof(button_b_state));		
		}
			
	}//end of while 1
}//end of main
