/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Omar Irshad
	UIN: 528000029
	Date: 2/1/2022
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/wait.h>
#include <fstream>
#include <chrono>

using namespace std;
using namespace std::chrono;


int main (int argc, char *argv[]) {
	int opt;
	int p = 0; //patient
	double t = -1; //time
	int e = 0; //ecg
	// MESSAGE_TYPE  m = 0;
	int c = 0;
	int buffer_capacity = MAX_MESSAGE; //buffer capacity set to 256 GB
	//char * buffer_cap_char;
	bool filereq = false;

	//We want to Run the server as a child process
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				filereq = true;
				break;
			case 'm' :
				buffer_capacity = atoi (optarg); //set mem passed in as buffer capacity
				// buffer_cap_char = optarg;
				break;
			case 'c' :
				c = 1;
				break;
				//add m and c 
		} 
	}
	//Task 1
	int value = fork(); 
	if(value == 0) //child
	{
		char *args[] = {const_cast<char *>("./server"), const_cast<char *>("-m"), const_cast<char *>(to_string(buffer_capacity).c_str()), nullptr};
		execvp(args[0],args); //Run the client as a child process
	}
	else //Parent
	{
		vector<FIFORequestChannel*> chans; //create a vector that holds on the potential channels
		FIFORequestChannel original("control", FIFORequestChannel::CLIENT_SIDE); //create channel;
		chans.push_back(&original); //push back chan
		if(c == 1){ //Wants to create a new channel
			//Opening a new channel 
			//Is the C channel a bool and how do I open it
			//Sending the command arugments in the new channel?
			MESSAGE_TYPE new_channel = NEWCHANNEL_MSG;
			char newbuf[30]; // create a new buffer of size 30 for the new_hcannnel to write to
			original.cwrite(&new_channel, sizeof(MESSAGE_TYPE));
			original.cread(newbuf, 30);

			//This FIFOREquestCHannel 
			//create chan_new on the heap
			FIFORequestChannel* chan_new = new FIFORequestChannel(newbuf, FIFORequestChannel::CLIENT_SIDE);
			chans.push_back(chan_new); //push back the latest channel in the hcannel
		}
		FIFORequestChannel chan = *(chans.back()); //sets the current chanel to the latest pushed back channel
		if((e!= 0) && (t != -1) && (p != 0)) //Server requests data points for a specific server
		{
		//Task2 part 1: Requesting Datapoints
			char buf[MAX_MESSAGE]; // 256
			datamsg x(p, t, e);
			memcpy(buf, &x, sizeof(datamsg));
			chan.cwrite(buf, sizeof(datamsg)); // question
			double reply;
			chan.cread(&reply, sizeof(double)); //answer
			cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
		}
		else if((e == 0) && (t == -1) && (p != 0)){ //Server request data point for multiple points
			//Need to open this csv file in the BIMDC file
			std::ofstream multi_points;
			multi_points.open ("received/x1.csv");
			auto start = high_resolution_clock::now(); //Time start now

			for (double i = 0; i <= 4 ; i = i + 0.004)// THe first 1,000 points goes up to time 4 seconds, therefore we increment by 0.004
			{ 
				multi_points << i <<","; 
				//For the first ECG value
				char buf[MAX_MESSAGE]; // 256
				e = 1;
				datamsg a(p, i, e);
				memcpy(buf, &a, sizeof(datamsg));
				chan.cwrite(buf, sizeof(datamsg)); // question
				double reply1;
				chan.cread(&reply1, sizeof(double)); //answer
				multi_points << reply1 << ",";

				//For the second ECG value:
				e = 2;
				datamsg b(p, i, e);
				memcpy(buf, &b, sizeof(datamsg));
				chan.cwrite(buf, sizeof(datamsg)); // question
				double reply2;
				chan.cread(&reply2, sizeof(double)); //answer
				multi_points << reply2;
				multi_points << endl;
		}
			auto stop = high_resolution_clock::now(); //Stop clock here 
			auto duration = duration_cast<microseconds>(stop - start);
  
    		cout << "Time taken by function: "
         	<< duration.count() << " microseconds" << endl;
			multi_points.close();	
		}

		//File request 
		if(filereq) // If a file name was passed through
		{
			filemsg fm(0,0);
			string fname = filename; //sets the name of the file to string
			__int64_t len = sizeof(filemsg) + (fname.size() + 1); 

			char* buf2 = new char[len]; //create buffer charecter array
			memcpy(buf2, &fm, sizeof(filemsg));
			strcpy(buf2 + sizeof(filemsg), fname.c_str()); //Buffer contains the file message
			chan.cwrite(buf2, len);  // I want the file length;

			__int64_t fileLength;
			chan.cread(&fileLength, sizeof(__int64_t));

			std::ofstream fout;
			string full_dir = "received/" + string(filename); //full directory

			fout.open(full_dir, std::ostream::binary); //This should put the file in the reciived


			

			__int64_t offset = 0; //initial offset
			char* new_buffer = new char[buffer_capacity];
			__int64_t size = buffer_capacity;

			// cout << "filename: " << fname << endl;
			// cout << "Length: " << fileLength << endl;
			// cout << "buffer cap: " << buffer_capacity << endl;
			// cout << "offset: " << offset << endl;

			auto start = high_resolution_clock::now(); //Time start now
			while( offset < fileLength)
			{
				if(buffer_capacity > (fileLength-offset)) //remainder
				{
					break;
				}
				
				filemsg init = filemsg(offset,size);

				memcpy(buf2, &init , sizeof(filemsg));
				strcpy(buf2 + sizeof(filemsg), fname.c_str());
				chan.cwrite(buf2, len);

				chan.cread(new_buffer,buffer_capacity);

				fout.write(new_buffer,buffer_capacity);
				offset += buffer_capacity;

			//file length
			}	
			__int64_t remainder = fileLength - offset;
			cout << "remainder: " << remainder << endl;
			if(remainder != 0) //If we havve only transfered a little under 
			{
				
				filemsg init = filemsg(offset,remainder); //create file message
				memcpy(buf2, &init, sizeof(filemsg));
				strcpy(buf2 + sizeof(filemsg), fname.c_str());
				chan.cwrite(buf2, len);
				chan.cread(new_buffer,remainder);
				fout.write(new_buffer,remainder);
			}
			auto stop = high_resolution_clock::now(); //Stop clock here 
			auto duration = duration_cast<microseconds>(stop - start);
  
    		cout << "Time taken by function: "
         	<< duration.count() << " microseconds" << endl;
			//delete and close  all buffers:
			fout.close();
			delete[] buf2;
			delete [] new_buffer;
			
		}
  
		// sending a non-sense message, you need to change this
		/*
		filemsg fm(0, 0);
		string fname = "teslkansdlkjflasjdf.dat";
		
		int len = sizeof(filemsg) + (fname.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		chan.cwrite(buf2, len);  // I want the file length;

		delete[] buf2;
		*/
		// closing the channels, if we created a new channel
		MESSAGE_TYPE m = QUIT_MSG;
		if(c == 1) 
		{
			for(long unsigned int i = 1; i < chans.size(); i++)
			{
				chans[i]->cwrite(&m, sizeof(MESSAGE_TYPE));
				delete chans[i]; // close everything in the child
				
			}
		}
		original.cwrite(&m, sizeof(MESSAGE_TYPE));
		wait(0);
	}
}
