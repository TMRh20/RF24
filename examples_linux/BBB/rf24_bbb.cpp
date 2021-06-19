/**
 * 
 * RF24 for BBB
 *
 * This program provide a way to help BBB developer easy to use RF24 library.
 *
 */

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <RF24/RF24.h>

using namespace std;

/****************** Linux (BBB,x86,etc) ***********************/

// See http://tmrh20.github.io/RF24/pages.html for more information on usage
// See http://iotdk.intel.com/docs/master/mraa/ for more information on MRAA
// See https://www.kernel.org/doc/Documentation/spi/spidev for more information on SPIDEV

// Setup for ARM(Linux) devices like BBB using spidev (default is "/dev/spidev1.0" )
//RF24 radio(115,0);

//BBB Alternate, with mraa
// CE pin = (Header P9, Pin 13) = 59 = 13 + 46 
//Note: Specify SPI BUS 0 or 1 instead of CS pin number. 
//RF24 radio(59,0);
// CE pin = P9_16
RF24 radio(51,0);

/********** User Config *********/
// Assign a unique identifier for this node, 0 or 1
bool radioNumber = 1;
const int min_payload_size = 4;
const int max_payload_size = 32;
char receive_payload[max_payload_size+1];
extern char *optarg;
/********************************/
int channel = 1;

void setWritingPipe(char *Addr) 
{
    radio.openWritingPipe((uint8_t*) Addr);
    cout << "WRITE: " << Addr << endl;
    cout.flush();
}

void setReadingPipe(char *Addr, int pipe) 
{
    radio.openReadingPipe(pipe, (uint8_t*) Addr);
    cout << "READ: " << Addr << endl;
    cout.flush();
}

void sendData(char *data) 
{
    int length = strlen(data);
    bool timeout = false;
    radio.stopListening();
    
    bool isSent = radio.write( data, length );
    
    if(!isSent) 
    {
        printf("{\"status\":\"failed\"}");
        return;
    }
    
    radio.startListening();
    unsigned long started_time = millis();
    
    while (!radio.available() && !timeout) 
    {
        if (millis() - started_time > 200 ) 
        {
    		timeout = true;
        }
    }
    
    if ( timeout )
	{
		printf("{\"status\":\"timeout\"}");
	} else {
	    printf("{\"status\":\"success\"}");
	}
    fflush(stdout);
}

void readData(char *setTimeout) 
{
    // Dump the payloads until we've gotten everything
    unsigned int len, waiting_time;
    bool timeout = false;
    
    if ( setTimeout > 0)
        waiting_time = atoi(setTimeout);
    else
        waiting_time = 200;
        
    radio.startListening();
    unsigned long started_time = millis();
    

    while (!radio.available() && !timeout)
    {
        if (millis() - started_time > waiting_time ) 
        {
    		timeout = true;
        }
    }
    
    if( timeout )
    {
        printf("{\"status\":\"timeout\"}");
    } 
    else 
    {
        // Fetch the payload, and see if this was the last one.
        len = radio.getDynamicPayloadSize();
        radio.read( receive_payload, len );
        
        // Put a zero at the end for easy printing
        receive_payload[len] = 0;
        
        printf("{\"status\":\"received\", \"length\":\"%i\", \"value\":\"%s\"}",len,receive_payload);
        fflush(stdout);
        
        radio.stopListening();
		
		radio.write( "R", 1 );

		// Now, resume listening so we catch the next packets.
		radio.startListening();
    }
}

void showHelp() 
{
    cout << "Usage: r24lib [-w target] [[-c channel] -r read_addr] [-options...]" << endl;
    cout << "-D\tshow module detail" << endl;
    cout << "-h\tshow this help" << endl;
    cout << "-w target\tset target address" << endl;
    cout << "-r read addr\tset read address" << endl;
    cout << "-c channel\tset read channel" << endl;
    cout << "-R[timeout]\tread string" << endl;
    cout << "-S string\tsend string" << endl;
    cout.flush();
}


int main(int argc, char** argv)
{
    
    // Setup and configure rf radio
    radio.begin();
    radio.enableDynamicPayloads();
    // optionally, increase the delay between retries & # of retries
    radio.setRetries(15,15);
    
    int c;
    while((c = getopt( argc, argv, "Dhw:r:S:c:R::"))!=-1) 
    {   
        switch(c) 
        {
            case 'w': setWritingPipe(optarg);
                      break;
            case 'r': setReadingPipe(optarg, channel);
                      break;
            case 'S': sendData(optarg);
                      break;
            case 'D': radio.printDetails();
                      break;
            case 'R': readData(optarg);
                      break;
            case 'h': showHelp();
                      break;
            case 'c': channel = 1;
                      break;
            case '?': fprintf(stderr, "Illegal option:-%c\n", isprint(optopt)?optopt:'#');
                      showHelp();
                      break;
            default: showHelp();
                     break;
        }
    }

    fflush(stdout);
    radio.startListening();
    
    return 0;
}