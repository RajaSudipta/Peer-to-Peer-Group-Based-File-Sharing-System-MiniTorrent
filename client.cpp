#ifndef HEADERS_H
#define HEADERS_H
#include <bits/stdc++.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <thread>
#include <openssl/sha.h>
#include <fstream>
#include <sys/stat.h>
#include <stdlib.h>
#include <libgen.h>
using namespace std;
#define MAX_CLIENTS 20
#define MAX_BUFFER_SIZE 500000
#define KB(x)   ((size_t) (x) << 10)
#endif

// g++ client.cpp -o client -lssl -lcrypto -pthread
// ./client 127.0.0.1:4000 tracker_info.txt
// upload_file /home/sudipta/Desktop/OS2/tracker_info.txt 1
// download_file 1 tracker_info.txt /home/sudipta/Desktop/OS2/test1.txt

// upload_file /home/sudipta/Desktop/OS2Mod/sample3.txt 1
// download_file 1 sample3.txt /home/sudipta/Desktop/OS2Mod/test2.txt


unordered_map<string,string> fileNameToPath;
string userName;
vector<string> downloading_contents;
bool isLoggedIn;
int tracker_sock;

void session_logged_in(string str)
{
	isLoggedIn = true;
	userName = str;
}

void clear_buffer(char* buffer,int size)
{
	memset(buffer, '\0', size);
}

void session_logged_out()
{
	fileNameToPath.clear();
	isLoggedIn = false;
	userName = "";
}

vector<string> tokenize_buffer(char * buffer)
{
	vector<string> tokens;
	char *token = strtok(const_cast<char*>(buffer), " ");
	while(token != NULL)
	{
		string temp = token;
		tokens.push_back(token);
		token = strtok(NULL," ");
	}
	return tokens;
}

void serving_utility(int new_socket)
{
	// cout<<"Yo\n";
	char buffer[MAX_BUFFER_SIZE];
	int numberOfBytesRecv = read(new_socket, buffer, 100000);

	if (numberOfBytesRecv == -1)
	{
		perror("Reading error");
		exit(EXIT_FAILURE);
	}
	else if (numberOfBytesRecv == 0)
	{
		cout << new_socket << " peer has performed an orderly shutdown!" << endl;
		return;
	}
	
	cout<<"Msg read in serving_utility: "<<buffer<<endl;
	// vector<string> cmds;
	// char *token = strtok(const_cast<char*>(buff), " ");
	// while(token != NULL){
	// 	string temp = token;
	// 	cmds.push_back(token);
	// 	token = strtok(NULL," ");
	// }

	vector<string> tokens = tokenize_buffer(buffer);
	clear_buffer(buffer,MAX_BUFFER_SIZE);
	cout << "After tokenizing" << endl;
	for(auto it: tokens)
	{
		cout << it << " ";
	}
	cout << endl;

	string fileName = tokens[1];

	if(tokens[0] == "bring")
	{
		int chunk_no = stoi(tokens[2]);
		string req_file = fileNameToPath[fileName];
		cout << "Req file path: " << req_file << endl;
		ifstream fin;
		fin.open(req_file);
		fin.seekg(KB(512)*(chunk_no-1));
		size_t chunk_size = KB(512);
		cout << "Chuk Size: " << chunk_size << endl;
		char * chunk = new char[chunk_size];
		fin.read(chunk,chunk_size);
		cout<<"actually read = "<<fin.gcount()<<endl;
		string size_of_chunk = to_string(fin.gcount());
		char to_send[fin.gcount()];
		bool leave_flag = false;
		for(int i=0;i<fin.gcount();i++)
		{
			to_send[i] = chunk[i];
		}
		send(new_socket,size_of_chunk.c_str(),size_of_chunk.size(),0);
		send(new_socket,to_send,fin.gcount(),0);
	}
	else
	{
		cout<<"Invalid msg from peer\n";
		bool leave_flag = false;
	}
}

void handle_other_peers_by_making_server(string IP, string port)
{
	struct  sockaddr_in addr;
	int sock_fd;
	int opt = 1;
	if((sock_fd = socket(AF_INET,SOCK_STREAM,0)) == 0){
		perror("Socket failed");
		return;
	}
	else
	{
		cout << "[+]Socket created successfully" << endl;
	}
	if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,&opt, sizeof(opt))){ 
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	else
	{
		cout << "[+]Socket attached to port: " << port << " successfully" << endl;
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(stoi(port)); 

	if (bind(sock_fd, (struct sockaddr *)&addr,sizeof(addr))<0){ 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	}
	else
	{
		cout << "[+]Socket binding to port: " << port << " successful" << endl;
	}

	if (listen(sock_fd, MAX_CLIENTS) < 0){ 
		perror("listen");
		exit(EXIT_FAILURE);
	}

	int addrlen = sizeof(addr);
	int new_socket;

	vector<thread> servingPeerThreads;

	while(true)
	{
		if ((new_socket = accept(sock_fd, (struct sockaddr *)&addr,(socklen_t*)&addrlen))<0)
		{
			perror("accept");
			exit(EXIT_FAILURE); 
		}
		bool leave_flag = false;
		cout << "connected" << endl;
		servingPeerThreads.push_back(thread(serving_utility, new_socket));
	}

}

bool quit_fn(bool flag)
{
	while(true)
	{
		if(flag == false)
		{
			return false;
		}
	}
	return true;
}

void download_chunk(string file_name,string sha1_of_this_chunk,int chunk_no, string IP, string port,string destination){
	struct sockaddr_in serv_addr; 
	int peer_socket = 0;
	if ((peer_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){ 
		printf("\n Socket creation error \n"); 
		return ; 
	} 
	else
	{
		cout << "[+]Socket created successfully" << endl;
	}
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(stoi(port.c_str())); 
	
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, IP.c_str(), &serv_addr.sin_addr)<=0) { 
		printf("\nInvalid address/ Address not supported \n"); 
		return; 
	}
	else
	{
		cout << "[+]addresses converted from text to binary form" << endl;
	}
	if (connect(peer_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){ 
		printf("\nConnection Failed \n"); 
		return; 
	}
	else
	{
		cout << "[+]Connection successful" << endl;
	}

	string msg = "bring "+file_name+" ";
	msg += to_string(chunk_no);
	send(peer_socket,msg.c_str() ,msg.size(),0);

	char size_of_chunk[30];
	read(peer_socket,size_of_chunk,30);
	int size_ofchunk = stoi(size_of_chunk);
	char buffer[size_ofchunk];
	clear_buffer(buffer,size_ofchunk);
	
	int bytes_read = read(peer_socket,buffer,size_ofchunk);
	cout<<"no. of bytes_read = "<<bytes_read<<endl;
	// cout<<"buff = "<<buff<<endl;
	cout<<"writing chunk_no "<<chunk_no<<endl;
	ofstream outfile;
	if(chunk_no == 1)
	{
		outfile.open(destination);
		outfile.write(buffer, size_ofchunk);		
	}
	else
	{
		outfile.open(destination, ios_base::app);
		outfile.write(buffer,size_ofchunk);
	}
	outfile.close();

}

void download(string file_name, vector<pair<string,string>>ip_ports, vector<string> original_sha1, string destination_path)
{
	int peers_available = ip_ports.size();
	cout<<"peers_available for this file = "<<peers_available<<endl;
	int no_of_chunks = original_sha1.size();
	cout<<"no_of_chunks = "<<no_of_chunks<<endl;
	downloading_contents.push_back(file_name);
	cout<<"Downloading... "<<file_name<<endl;
	if(no_of_chunks <= peers_available)
	{
		vector<thread> fetch_chunks;
		for(int i = 0; i < no_of_chunks; i++)
		{
			fetch_chunks.push_back(thread(download_chunk,file_name,original_sha1[i], i+1 , ip_ports[i].first,ip_ports[i].second,destination_path));
		}
		while(fetch_chunks.size() > 0)
		{
			fetch_chunks[0].join();
			fetch_chunks.erase(fetch_chunks.begin());
		}
	}
	else
	{
		int chunk_per_peer = no_of_chunks/peers_available;
		vector<thread> fetch_chunks;
		int chunk_no = 1;
		int c_chunk = 0;
		for(int i=0;i<peers_available;i++)
		{
			c_chunk *= 2;
			for (int j = 0; j < chunk_per_peer; j++)
			{
				c_chunk++;
				fetch_chunks.push_back(thread(download_chunk,file_name,original_sha1[chunk_no-1], chunk_no , ip_ports[i].first,ip_ports[i].second,destination_path));
				cout<<"chunk_no = "<<chunk_no<<endl;
				chunk_no++;
			}
		}

		int remaining = no_of_chunks - (chunk_per_peer * peers_available);
		c_chunk++;
		for(int i = 0; i<remaining;i++)
		{
			fetch_chunks.push_back(thread(download_chunk,file_name,original_sha1[chunk_no-1], chunk_no , ip_ports[i].first,ip_ports[i].second,destination_path));
			chunk_no++;			
		}
		while(fetch_chunks.size() > 0)
		{
			fetch_chunks[0].join();
			fetch_chunks.erase(fetch_chunks.begin());
		}
	}

	auto it = find(downloading_contents.begin(),downloading_contents.end(),file_name);
	downloading_contents.erase(it);
	fileNameToPath[file_name] = destination_path;
	cout<< "Download Complete for " <<file_name<<endl;
	return;
}


string send_and_recieve_message_from_server(int socket, string str)
{
	int y = 0;
	send(socket , str.c_str() , str.size() , 0 );
	char buffer[MAX_BUFFER_SIZE];
	y = 0;
	clear_buffer(buffer,MAX_BUFFER_SIZE);
	
	int numberOfBytesRecv = read(socket,buffer,MAX_BUFFER_SIZE);
	if (numberOfBytesRecv == -1)
	{
		perror("Reading error");
		exit(EXIT_FAILURE);
	}
	else if (numberOfBytesRecv == 0)
	{
		string s = "";
		cout << socket << " peer has performed an orderly shutdown!" << endl;
		return s;
	}

	string ans = buffer;
	return ans;
}

long long int getFileSize(string filename)
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    // return rc == 0 ? stat_buf.st_size : -1;
	if(rc == 0)
	{
		return stat_buf.st_size;
	}
	else
	{
		cout << "File Not Found!" << endl;
		return -1;
	}
}

string getFileNamefromPath(string path)
{
	vector<string> tokens;
	stringstream check1(path);
	string intermediate;
	
	// Tokenizing w.r.t. colon ' '
	while(getline(check1, intermediate, '/'))
	{
		tokens.push_back(intermediate);
	}
	string last_segment = tokens.back();
	return last_segment;
}


int main(int argc, char const *argv[]) 
{ 
	if(argc != 3)
    {
        cout << "Invalid number of arguements" << endl;
        cout << "./client <IP>:<PORT> tracker_info.txt" << endl;
        exit(EXIT_FAILURE);
    }

	/* log out */
	// logged_out();
	isLoggedIn = false;
    userName.clear();
	fileNameToPath.clear();

	/* extracting tracker ip and port */
    ifstream inFile(argv[2]);
    string tracker_ip, tracker_port;
    inFile >> tracker_ip >> tracker_port;
    cout << "Tracker IP: " << tracker_ip << ", Port: " << tracker_port << endl;

    /* extracting client ip and port */
    string clientIpPort = argv[1];
    string client_ip, client_port;
    vector <string> tokens;
    stringstream check1(clientIpPort);
    string intermediate;
      
    // Tokenizing w.r.t. colon ':'
    while(getline(check1, intermediate, ':'))
    {
        tokens.push_back(intermediate);
    }
    client_ip = tokens[0];
    client_port = tokens[1];
    cout << "Client IP: " << client_ip << ", Port: " << client_port << endl;

	tracker_sock = 0; 
	struct sockaddr_in serv_addr; 
	if ((tracker_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{ 
		printf("\n Socket creation error \n"); 
		return -1; 
	} 
	else
    {
        cout << "[+]Socket created successfully!" << endl;
    }

	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(stoi(tracker_port.c_str())); 
	
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, tracker_ip.c_str(), &serv_addr.sin_addr)<=0) 
	{ 
		printf("\nInvalid address/ Address not supported \n"); 
		return -1; 
	}
	else
    {
        cout << "[+]Address converted to binary form successfully" << endl;
    }
	if (connect(tracker_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){ 
		printf("\nConnection Failed \n"); 
		return -1; 
	}
	else
    {
        cout << "[+]Connection Successful" << endl;
    }

	thread thrd(handle_other_peers_by_making_server,client_ip,client_port);

	while(true)
	{
		string command;
		getline(cin,command);
		vector<string> tokens;
        stringstream check1(command);
        string intermediate;
        
        // Tokenizing w.r.t. colon ' '
        while(getline(check1, intermediate, ' '))
        {
            tokens.push_back(intermediate);
        }
        if(tokens.size() == 0)
        {
            continue;
        }
			
		if(tokens[0] == "create_user")
		{
			if(tokens.size() != 3)
			{
				cout << "Invalid args to create_user" << endl;
				continue;
			}
			string user_name = tokens[1];
			string passwd = tokens[2];
			string msg = "create_user " + user_name + " " + passwd;
			string reply_from_tracker = send_and_recieve_message_from_server(tracker_sock, msg);
			cout << reply_from_tracker << endl;
		}
		else if(tokens[0] == "login")
		{
			string user_name = tokens[1];
			string passwd = tokens[2];
			if(tokens.size() != 3)
			{
				cout<<"Invalid args to login\n";
				continue;
			}
			if(isLoggedIn == true)
			{
				cout<<"Logging out "<<userName<<endl;
				string msg = "logout " + userName; 
				string reply = send_and_recieve_message_from_server(tracker_sock, msg);
				cout<<reply<<endl;
				int x=0;
				/* session log out */
				session_logged_out();
			}
			string msg = "login " + user_name + " " + passwd + " " + client_ip + " " + client_port;
			string reply = send_and_recieve_message_from_server(tracker_sock, msg);
			int duration =12;
			if(reply[0] == 'L' && reply[1] == 'o')
			{
				/* session log out */
				session_logged_out();
				/* session log in */
				session_logged_in(user_name);
				cout<<user_name<<" logged in!"<<endl;

				vector<string> reply_tokens;
				stringstream check1(reply);
				string intermediate;
				
				// Tokenizing w.r.t. colon ' '
				while(getline(check1, intermediate, ' '))
				{
					reply_tokens.push_back(intermediate);
				}
				
				for(int i=1; i<reply_tokens.size(); i=i+2)
				{
					string fname = reply_tokens[i];
					string fPath = reply_tokens[i+1];
					cout<<fname<<" -> "<<fPath<<endl;
					fileNameToPath[fname] = fPath;
				}
			}
			else
			{
				string temp = reply;
				cout<<reply<<endl;
			}
		}
		else if(tokens[0] == "logout")
		{
			if(tokens.size() != 1)
			{
				cout << "Invalid args to <logout>" << endl;
				continue;
			}
			if(isLoggedIn == false)
			{
				cout<<"No User Logged in" << endl;
				continue;
			}
			string msg = "logout " + userName;
			string reply_from_server = send_and_recieve_message_from_server(tracker_sock, msg);
			cout << reply_from_server << endl;
			/* session log out */
			session_logged_out();
		}

		else if(tokens[0] == "create_group")
		{
			if(tokens.size() != 2)
			{
				cout << "Invalid args to <create_group>" << endl;
				continue;
			}
			string temp = "logged";
			if(isLoggedIn == false)
			{
				cout <<"No User Logged in" <<endl;
				continue;
			}
			string groupId = tokens[1];
			string msg = "create_group " + groupId + " " + userName;
			string reply_from_server = send_and_recieve_message_from_server(tracker_sock, msg);
			cout << reply_from_server << endl;
		}
		else if(tokens[0] == "join_group")
		{
			if(tokens.size() != 2)
			{
				cout<<"Invalid args to <join_group>" << endl;
				continue;
			}
			string temp = "logged";
			if(isLoggedIn == false)
			{
				cout<<"No User Logged in" << endl;
				continue;
			}
			string groupId = tokens[1];
			string msg = "join_group " + groupId + " " + userName;
			string reply_from_server = send_and_recieve_message_from_server(tracker_sock, msg);
			cout << reply_from_server << endl;			
		}
		else if(tokens[0] == "leave_group")
		{
			if(tokens.size() != 2)
			{
				cout << "Invalid args to <leave_group>" << endl;
				continue;
			}
			string temp = "logged";
			if(isLoggedIn == false)
			{
				cout << "No User Logged in" << endl;
				continue;
			}
			string groupId = tokens[1];
			string msg = "leave_group " + groupId + " " + userName;
			string reply_from_server = send_and_recieve_message_from_server(tracker_sock, msg);
			cout << reply_from_server << endl;			
		}
		else if(tokens[0] == "list_requests")
		{
			if(tokens.size() != 2)
			{
				cout << "Invalid args to <list_requests>" << endl;
				continue;
			}
			string temp = "logged";
			if(isLoggedIn == false)
			{
				cout<<"No User Logged in" << endl;
				continue;
			}
			string groupId = tokens[1];
			string msg = "list_requests " + groupId + " " + userName;
			string reply_from_server = send_and_recieve_message_from_server(tracker_sock, msg);
			cout<<reply_from_server<<endl;			
		}
		else if(tokens[0] == "accept_request")
		{
			if(tokens.size() != 3)
			{
				cout << "Invalid args to <accept_request>" << endl;
				continue;
			}
			string temp = "logged";
			if(isLoggedIn == false)
			{
				cout<<"No User Logged in"<<endl;
				continue;
			}
			string groupId = tokens[1];
			string userId = tokens[2];
			string msg = "accept_request " + groupId + " " + userId + " " + userName;
			string reply_from_server = send_and_recieve_message_from_server(tracker_sock, msg);
			cout<<reply_from_server<<endl;			
		}
		else if(tokens[0] == "list_groups")
		{
			if(tokens.size() != 1){
				cout<<"Invalid args to <list_groups>\n";
				continue;				
			}
			string temp = "logged";
			if(isLoggedIn == false)
			{
				cout<<"No User Logged in" << endl;
				continue;
			}
			string msg = "list_groups";
			string reply_from_server = send_and_recieve_message_from_server(tracker_sock, msg);
			cout<<reply_from_server<<endl;						
		}
		else if(tokens[0] == "list_files")
		{
			if(tokens.size() != 2)
			{
				cout << "Invalid args to <list_files>" << endl;
				continue;				
			}
			string temp = "logged";
			if(isLoggedIn == false)
			{
				cout << "No User Logged in" << endl;
				continue;
			}
			string groupId = tokens[1];
			string msg = "list_files " + groupId + " " + userName;
			string reply_from_server = send_and_recieve_message_from_server(tracker_sock, msg);
			cout<<reply_from_server<<endl;						
		}

		else if(tokens[0] == "upload_file")
		{
			if(tokens.size() != 3)
			{
				cout<<"Invalid args to <upload_file>" << endl;
				continue;
			}
			if(isLoggedIn == false)
			{
				cout<<"No User Logged in" << endl;
				continue;
			}
			string filePath = tokens[1];
			string groupId = tokens[2];
			long long int file_size = getFileSize(filePath);
			if(file_size == -1)
			{
				cout << "File Size error" << endl;
			}
			else
			{
				string file_name = getFileNamefromPath(filePath);
				fileNameToPath[file_name] = filePath;
				cout << "File Name : " << file_name << endl;
				cout << "File size = " << file_size << endl;
				size_t chunk_size = KB(512);
				cout<<"Chunk_size = "<<chunk_size<<endl;
				char * chunk = new char[chunk_size];
				string imp = "logged";
				vector<string> sha1_vector;
				ifstream fin;
				fin.open(filePath);
				ifstream f2in2;
				while(fin)
				{
					fin.read(chunk, chunk_size);
					imp += " ";
					if(!fin.gcount())
					{
						break;
					}
					char buff[MAX_BUFFER_SIZE];
					clear_buffer(buff, MAX_BUFFER_SIZE);

					unsigned char hash[SHA_DIGEST_LENGTH]; // == 20
					SHA1(reinterpret_cast<const unsigned char *>(chunk), sizeof(chunk) - 1, hash);
					unsigned char hash22[SHA_DIGEST_LENGTH];
					string c_hash(reinterpret_cast<char*>(hash));
					sha1_vector.push_back(c_hash);
					imp += "logged";
					cout << hash << endl;
				}
				string groupId = tokens[2];
				string msg = "upload_file " + groupId + " " + userName + " " + file_name + " " + filePath + " ";
				msg += to_string(file_size) + " ";
				vector<string>:: iterator it;
				for(it=sha1_vector.begin(); it!=sha1_vector.end(); it++)
				{
					msg += *it;
					msg += " ";
				}
				string reply_from_server = send_and_recieve_message_from_server(tracker_sock, msg);
				cout << reply_from_server << endl;
			}
		}
		
		else if(tokens[0] == "download_file")
		{
			if(tokens.size() != 4)
			{
				cout<<"Invalid args to <download_file>" << endl;
				continue;
			}
			string imp = "logged";
			if(isLoggedIn == false)
			{
				cout<<"No User Logged in" <<endl;
				continue;
			}
			string file_name22 = tokens[2];
			string group_id = tokens[1];
			string file_name = tokens[2];
			string destination_path = tokens[3];
			string msg = "download_file " + file_name + " " + group_id + " " + userName;
			string reply_from_server = send_and_recieve_message_from_server(tracker_sock, msg);

			cout << "Reply from server: " << reply_from_server << endl;

			if(reply_from_server[0] == '@')
			{
				cout << "You are not a member of the group" << endl;
				continue;
				
			}
			if(reply_from_server[0] == '~')
			{
				cout << "File not found in the Group" << endl;
				continue;	

			}

			/* check */
            vector<string> reply_tokens;
            stringstream check1(reply_from_server);
            string intermediate;
            while(getline(check1, intermediate, ' '))
            {
                reply_tokens.push_back(intermediate);
            }
            
			cout << "after tokenization reply tokens are" << endl;
			for(auto it: reply_tokens)
			{
				cout << it << " ";
			}
			cout << endl;

            int i=0, pos = 0;
            vector<string> sha1_vector;
            while(i<reply_tokens.size())
            {
                if(reply_tokens[i] == ";")
                {
                    pos = i;
                    break;
                }
                else
                {
                    sha1_vector.push_back(reply_tokens[i]);
                }
				i++;
            }
			cout << "Pos of ; " << pos << endl;
            vector<pair<string, string>> serving_ip_ports;
            for(i=pos+1; i<reply_tokens.size(); i=i+2)
            {
                serving_ip_ports.push_back(make_pair(reply_tokens[i], reply_tokens[i+1]));
            }
            int no_of_ip_ports = serving_ip_ports.size();
            if(no_of_ip_ports == 0)
            {
                cout << "No peer is serving the file" << endl;
                continue;
            }
            vector<pair<string, string>>:: iterator it;
            cout << "Serving IP and ports are" << endl;
            for(it=serving_ip_ports.begin(); it!=serving_ip_ports.end(); it++)
            {
                cout << it->first << " " << it->second << endl;
            }

            cout << "The original SHA1 are" << endl;
            for(auto it:sha1_vector)
            {
                cout << it << endl;
            }

            vector<thread> curr_downloaders;
            string junk;
            curr_downloaders.push_back(thread(download, file_name, serving_ip_ports, sha1_vector, destination_path));
            junk = "kl";
            while(curr_downloaders.size() > 0)
            {
                junk = "kl";
                curr_downloaders[0].join();
                junk = "kl";
                curr_downloaders.erase(curr_downloaders.begin());
            }

		}

		else if(tokens[0] == "show_downloads")
		{
			if(tokens.size() != 1){
				cout << "Invalid args to <show_downloads>" << endl;
				continue;
			}
			string temp = "logged";
			if(isLoggedIn == false)
			{
				cout<<"No User Logged in" << endl;
				continue;
			}
			temp = "logged";
			unordered_map<string, string>:: iterator it;
			for(it = fileNameToPath.begin(); it != fileNameToPath.end(); it++)
			{
				cout<<"[C] "<<it->first<<endl;
			}
			vector<string>:: iterator it2;
			for(it2=downloading_contents.begin(); it2!=downloading_contents.end(); it2++)
			{
				cout<<"[D] "<<*it2<<endl;
			}
		}
		else if(tokens[0] == "stop_share")
		{
			if(tokens.size() != 3)
			{
				cout<<"Invalid args to <stop_share>\n";
				continue;
			}
			string temp = "logged";
			if(isLoggedIn == false)
			{
				cout<<"No User Logged in" << endl;
				continue;
			}
			string groupId = tokens[1];
			string file_name = tokens[2];
			string msg = "stop_share " + groupId + " " + file_name + " " + userName;
			string reply_from_server = send_and_recieve_message_from_server(tracker_sock, msg);
			cout<<reply_from_server<<endl;
		}
		else
		{
			cout << "Invalid Command" << endl;
		}
	}
	string imp = "logged";
	thrd.join();
	return 0; 
} 
