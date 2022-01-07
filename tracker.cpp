#include <bits/stdc++.h> 
#include <unistd.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <thread> 
#define ll long long
#define MAX_CLIENTS 20
#define MAX_BUFFER_SIZE 100000
using namespace std;

// g++ tracker.cpp  -o tracker -pthread
// ./tracker tracker_info.txt

class User
{
public:
	bool isLoggedIn;

	string user_name;
	string password;

	string serving_ip;
	string serving_port;
		
	unordered_map<string,string> fileNameToAbsolutePath;

	User(string user_name,string passwd)
	{
		isLoggedIn = false;
		user_name = user_name;
		password = passwd;
		
	}
	~User()
	{

	}
	void logout()
	{
		isLoggedIn = false;
		serving_ip = "";
		serving_port = "";
		
	}
	void login(string IP, string Port)
	{
		isLoggedIn = true;
		serving_ip = IP;
		serving_port = Port;
	}
	
};

unordered_map<string, User*> usersMap;

class FileDesc
{
public:
	vector<pair<string,bool>> userFileBitMap;
	vector<string> sha1;
	string tesp;	
	string file_name;
	long long int file_size;

	FileDesc(string name, long long int size)
	{
		file_size = size;
		file_name = name;
	}
	~FileDesc()
	{

	}
	bool isUser(string user)
	{
		for(auto it=userFileBitMap.begin(); it!=userFileBitMap.end(); it++){
			if(it->first == user)
			{
				return true;
			}
		}
		return false;
	}
	void addUser(string str)
	{
		userFileBitMap.push_back(make_pair(str, true));
	}
	
};

unordered_map<string, FileDesc*> filesMap;

class Group
{
public:
	string group_id;
	string owner;
	vector<string> members;
	vector<string> filesinGroup;
	vector<string> pendingRequests;
	Group(string gid, string ownername)
	{
		owner = ownername;
		group_id = gid;
		members.push_back(ownername);
	}
	~Group()
	{

	}
	bool doesFileBelongToGroup(string fl)
	{
		for(auto it=filesinGroup.begin(); it!=filesinGroup.end(); it++)
		{
			if(*it == fl)
			{
				return true;
			}
		}
		return false;
	}
	bool isReqPending(string mem)
	{
		for(auto it=pendingRequests.begin(); it!=pendingRequests.end(); it++)
		{
			if(*it == mem)
			{
				return true;
			}
				
		}
		return false;
	}
	bool isMember(string mem)
	{
		for(auto it=members.begin(); it!=members.end(); it++)
		{
			if(*it == mem)
			{
				return true;
			}
		}
		return false;
	}
	void accept_req(string mem)
	{
		auto it = find(pendingRequests.begin(),pendingRequests.end(),mem);
		pendingRequests.erase(it);
		members.push_back(mem);
	}
	void removeMember(string mem)
	{
		auto it = find(members.begin(),members.end(),mem);
		members.erase(it);
	}
	
};

unordered_map<string, Group*> groupsMap;


void clear_buffer(char* buffer,int size)
{
	memset(buffer, '\0', size);
}

bool userExists(string userName)
{
	if(usersMap.find(userName) != usersMap.end())
	{
		return true;
	}
	return false;
}

bool fileExists(string fileName)
{
	if(filesMap.find(fileName) != filesMap.end())
	{
		return true;
	}
	return false;
}

bool groupExists(string gname)
{
	if(groupsMap.find(gname) != groupsMap.end())
	{
		return true;
	}
	return false;
}

vector<string> tokenize_buffer(char * buffer)
{
	vector<string> tokens;
	char *token = strtok(buffer, " ");
	while(token != NULL)
	{
		string temp = token;
		tokens.push_back(token);
		token = strtok(NULL," ");
	}
	return tokens;
}

void handle_peers(int peer_fd)
{
	cout << "Listening to " << peer_fd << endl;
	while(true)
	{
		char buffer[MAX_BUFFER_SIZE];
		int numberOfBytesRecv = read(peer_fd, buffer, 100000);

		if (numberOfBytesRecv == -1)
        {
            perror("Reading error");
            exit(EXIT_FAILURE);
        }
        else if (numberOfBytesRecv == 0)
        {
            cout << peer_fd << " peer has performed an orderly shutdown!" << endl;
            return;
        }
		cout << "Received from " << peer_fd << ": " << buffer << endl;

		vector<string> tokens = tokenize_buffer(buffer);
		clear_buffer(buffer,MAX_BUFFER_SIZE);
		cout << "After tokenizing" << endl;
		for(auto it: tokens)
		{
			cout << it << " ";
		}
		cout << endl;

		/* Handle the commands */
		if(tokens[0] == "create_user")
		{
			string userName = tokens[1];
			string passwd = tokens[2];
			if(userExists(userName) == true)
			{
				string s = "User " + userName + " already exists";
				send(peer_fd, s.c_str() , s.length() , 0 );
			}
			else
			{
				User *newUser = new User(userName, passwd);
				usersMap[userName] = newUser;
				string s = "Created "+userName+" successfully!";
				send(peer_fd , s.c_str() , s.length() , 0);
				cout << "Created " << userName << endl;
			}
		}

		if(tokens[0] == "login")
		{
			string userName = tokens[1];
            string passwd = tokens[2];
            /* client will send two extra parametes to tracker */
            string userIp = tokens[3];
            string userPort = tokens[4];

			if(userExists(userName) == true)
			{
				User *curr_user = usersMap[userName];
				if(curr_user->password != passwd)
				{
					string s = "Incorrect Password";
					send(peer_fd, s.c_str() , s.length() , 0 );
				}
				else
				{
					curr_user->login(userIp, userPort);
					string msg = "Loggedin ";
					for(auto it : curr_user->fileNameToAbsolutePath)
					{
						msg += it.first + " ";
						msg += it.second + " ";
					}
					send(peer_fd, msg.c_str() ,msg.length() , 0 );
				}
			}
			else
			{
				string s = "User " + userName + " doesn't exist";
				send(peer_fd, s.c_str() , s.length() , 0);
			}
		}

		if(tokens[0] == "logout")
		{
			string userName = tokens[1];
			if(userExists(userName) == true)
			{
				usersMap[userName]->logout();
				string s = "Logged Out!";
				send(peer_fd, s.c_str(), s.length(), 0);
			}
			else
			{
				string s = "User " + userName + "doesn't exist";
				send(peer_fd, s.c_str(), s.length(), 0);
			}
		}

		if(tokens[0] == "create_group")
		{
			string groupId = tokens[1];
            /* We need owner name of group from peer as an extra parameter */
            string ownerName = tokens[2];

			if(userExists(ownerName) == false)
			{
				string s = "User " + ownerName + "does not exist";
				send(peer_fd, s.c_str() , s.length() , 0 );
			}
			else if(groupExists(groupId) == true)
			{
				string s = "Group" + groupId + " already exists";
				send(peer_fd, s.c_str() , s.length() , 0 );				
			}
			else
			{
				Group * newGroup = new Group(groupId, ownerName);
				groupsMap[groupId] = newGroup;
				string s = 	 "Group" + groupId +  " created successfully!";		
				send(peer_fd, s.c_str() , s.length() , 0 );				
			}
		}
		if(tokens[0] == "join_group")
		{
			string groupId = tokens[1];
            /* We need owner name of group from peer as an extra parameter */
            string userName = tokens[2];

			if(userExists(userName) == false)
			{
				string s = "User " + userName + "does not exist";
				send(peer_fd, s.c_str() , s.length() , 0 );
			}
			else if(groupExists(groupId) == false)
			{
				string s = "Group " + groupId + "does not exist";
				send(peer_fd, s.c_str() , s.length() , 0 );				
			}
			else if(groupsMap[groupId]->isMember(userName) == true)
			{
				string s = "You are already a member of this group " + groupId;
				send(peer_fd, s.c_str() , s.length() , 0 );				
			}
			else
			{
				groupsMap[groupId]->pendingRequests.push_back(userName);
				string s = "Request to join group sent!";
				send(peer_fd, s.c_str() , s.length() , 0 );				
			}
		}
		if(tokens[0] == "leave_group")
		{
			string groupId = tokens[1];
            /* We need owner name of group from peer as an extra parameter */
            string userName = tokens[2];

			if(userExists(userName) == false)
			{
				string s = "User does not exist";
				send(peer_fd, s.c_str(), s.length(), 0);
			}
			else if(groupExists(groupId) == false)
			{
				string s = "Group doesn't exist";
				send(peer_fd, s.c_str(), s.length(), 0);			
			}
			else if(groupsMap[groupId]->isMember(userName) == false)
			{
				string s = "You are not a member of this group";
				send(peer_fd, s.c_str(), s.length(), 0);	
			}
			else
			{
				groupsMap[groupId]->removeMember(userName);
				string s = "User " + userName + " left the group";
				send(peer_fd, s.c_str() , s.length() , 0 );				
			}
		}
		if(tokens[0] == "list_requests")
		{
			string groupId = tokens[1];
            /* We need owner name of group from peer as an extra parameter */
            string userName = tokens[2];

			if(userExists(userName) == false)
			{
				string s = "User " + userName + "doesn't exist";
				send(peer_fd, s.c_str() , s.length() , 0 );
			}
			else if(groupExists(groupId) == false)
			{
				string s = "Group doesn't exist";
				send(peer_fd, s.c_str() , s.length() , 0 );
			}
			else if(groupsMap[groupId]->owner != userName)
			{
				string msg = "You are not the owner of this group";
				send(peer_fd, msg.c_str() , msg.length() , 0 );				
			}
			else
			{
				string msg = "";
				Group *curr_group = groupsMap[groupId];
				if(curr_group->pendingRequests.size() == 0)
				{
					msg = "No pending requests";
				}
				else
				{
					for(auto it: curr_group->pendingRequests)
					{
						msg += it;
						msg += " ";
					}
				}
				send(peer_fd, msg.c_str() , msg.length() , 0 );				
			}
		}
		if(tokens[0] == "accept_request")
		{
			string group_id = tokens[1];
            /* We need userName and ownerName of group from peer as an extra parameter */
            string userName = tokens[2];
            string ownerName = tokens[3];

			if(userExists(ownerName) == false)
			{
				string s = "User does not exist";
				send(peer_fd, s.c_str() , s.length() , 0 );
			}
			else if(groupExists(group_id) == false)
			{
				string s = "Group doesn't exist";
				send(peer_fd, s.c_str() , s.length() , 0 );				
			}
			else if(groupsMap[group_id]->owner != ownerName)
			{
				string s = "You are not the owner of this group";
				send(peer_fd, s.c_str() , s.length() , 0 );				
			}
			else if(groupsMap[group_id]->isReqPending(userName) == false)
			{
				string s = "No pending request found for this user";
				send(peer_fd, s.c_str() , s.length() , 0 );							
			}
			else
			{
				Group *curr_group = groupsMap[group_id];
				curr_group->accept_req(userName);
				string s = "Request accepted";
				send(peer_fd, s.c_str() , s.length() , 0 );
			}
		}
		if(tokens[0] == "list_groups")
		{
			if(groupsMap.size() == 0)
			{
				string msg = "No Groups found!";
				send(peer_fd, msg.c_str() , msg.size() , 0 );
			}
			else
			{
				string msg = "";
				for(auto it : groupsMap)
				{
					msg += it.first;
					msg.push_back('\n');
				}
				send(peer_fd, msg.c_str() , msg.size() , 0 );
			}
		}

		if(tokens[0] == "list_files")
		{
			string groupId = tokens[1];
            /* We need userName of group from peer as an extra parameter */
            string userName = tokens[2];
			
			if(groupExists(groupId) == false)
			{
				string s = "Group doesn't exist";
				send(peer_fd, s.c_str() , s.length() , 0 );				
			}
			else if(groupsMap[groupId]->isMember(userName) == false)
			{
				string s = "You are not a member of this group";
				send(peer_fd, s.c_str() , s.length() , 0 );			
			}
			else
			{
				Group *curr_group = groupsMap[groupId];

				string msg = "";
				for(string s: curr_group->filesinGroup)
				{
					for(auto it=filesMap[s]->userFileBitMap.begin(); it!=filesMap[s]->userFileBitMap.end(); it++)
					{
						if(it->second == true)
						{
							msg += s;
							msg.push_back('\n');
						}
					}
				}
				if(msg == "")
				{
					msg = "No files in this group\n";
				}
				send(peer_fd, msg.c_str() , msg.length() , 0 );				
			}
		}

		if(tokens[0] == "upload_file")
		{
			string groupId = tokens[1];
            string userName = tokens[2];
            string fileName = tokens[3];
            string fileAbsolutePath = tokens[4];
            string file_size = tokens[5];

			if(userExists(userName) == false)
			{
				string s = "User " + userName + " doesn't exist";
				send(peer_fd, s.c_str() , s.length() , 0 );
			}
			if(groupExists(groupId) == false)
			{
				string s = "Group " + groupId +"doesn't exist";
				send(peer_fd, s.c_str() , s.length() , 0 );				
			}
			else
			{
				Group *curr_group = groupsMap[groupId];
				if(curr_group->doesFileBelongToGroup(fileName) == false)
				{
					curr_group->filesinGroup.push_back(fileName);
				}
				if(fileExists(fileName) == true)
				{
					FileDesc *curr_file = filesMap[fileName];
					if(curr_file->isUser(userName) == false)
					{
						curr_file->addUser(userName);
						User *curr_user = usersMap[userName];
						curr_user->fileNameToAbsolutePath[fileName] = fileAbsolutePath;
						string msg = fileName + " uploaded successfully";
						send(peer_fd, msg.c_str() , msg.length() , 0 );
					}
					else
					{
						string s = "File " + fileName + " already there";
						send(peer_fd, s.c_str(), s.length(), 0);
					}
				}
				else
				{
					FileDesc *new_file = new FileDesc(fileName, stoi(file_size));
					new_file->addUser(userName);
					filesMap[fileName] = new_file;
					User *curr_user = usersMap[userName];
					curr_user->fileNameToAbsolutePath[fileName] = fileAbsolutePath;
					int start = 6;
					int vsize = tokens.size();
					for(int i=start;i<vsize;i++)
					{
						new_file->sha1.push_back(tokens[i]);
					}
					string msg = fileName + " uploaded successfully";
					send(peer_fd, msg.c_str() , msg.length() , 0 );
				}
			}
		}

		if(tokens[0] == "download_file")
		{
            string file_name = tokens[1];
			string groupId = tokens[2];
            string userName = tokens[3];
            Group *curr_group = groupsMap[groupId];
			cout << "till here" << endl;
			// string msg = "";
			if(curr_group->isMember(userName) == false)
			{
				string s = "@";
				send(peer_fd, s.c_str() , s.length() , 0 );
				cout << "till here2" << endl;
				continue;				
			}
			if(curr_group->doesFileBelongToGroup(file_name) == false)
			{
				string s = "~";
				send(peer_fd, s.c_str(), s.length(), 0);
				cout << "till here3" << endl;
				continue;
			}
			FileDesc *curr_file = filesMap[file_name];
			string msg = "";
			cout << "till here3" << endl;
			for(auto it=curr_file->sha1.begin(); it!=curr_file->sha1.end(); it++)
			{
				msg += *it;
				msg += " ";
			}
			cout << "till here4" << endl;
			msg += "; ";
			cout << "till here5" << endl;
			for(auto it: curr_file->userFileBitMap)
			{
				User *curr_user = usersMap[it.first];
				if(it.second == true && curr_user->isLoggedIn == true)
				{	
					msg += curr_user->serving_ip+" ";
					msg += curr_user->serving_port+" ";
				}
			}
			cout << "till here5" << endl;
			send(peer_fd, msg.c_str(), msg.size(), 0);
		}

		if(tokens[0] == "stop_share")
		{
			string groupId = tokens[1];
            string fileName = tokens[2];
            /* client will pass an extra parameter userName to tracker */
            string userName = tokens[3];
			
			if(fileExists(fileName) == false)
			{
				string s = "File " + fileName + "doesn't exist";
				send(peer_fd, s.c_str() , s.length() , 0 );
			}
			else if(groupExists(groupId) == false)
			{
				string s = "Group doesn't exist";
				send(peer_fd, s.c_str() , s.length() , 0 );		
			}
			else if(groupsMap[groupId]->isMember(userName) == false)
			{
				string s = "You are not a member of this group";
				send(peer_fd, s.c_str() , s.length() , 0 );		
			}
			else
			{
				FileDesc *curr_file = filesMap[fileName];
				for(auto it=curr_file->userFileBitMap.begin(); it!=curr_file->userFileBitMap.end(); it++)
				{
					if(it->first == userName)
					{
						it->second = false;
						break;
					}
				}
				string s = "Stopped sharing";
				send(peer_fd, s.c_str() , s.length() , 0 );				
			}
		}

	}
}

void quit_fn(bool &isquit)
{
	string str;
	int i = 0;
	cin >> str;
	if(str == "quit")
	{
		isquit = true;
		i = 1;
		return;
	}
}


int main(int argc, char const *argv[]) 
{ 
	int server_fd;
	int new_socket;
	int valread; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 

	if(argc != 2)
	{
		cout << "Invalid number of args" << endl;
        cout << "./server tracker_info.txt" << endl;
        exit(EXIT_FAILURE);
	}

	// fstream file;
	// file.open(argv[1]);
	// string tracker_ip,tracker_port;
	// file >> tracker_ip;
	// file >> tracker_port;
	// cout<<"Serving at "<<tracker_ip<<":"<<tracker_port<<endl;

	ifstream inFile(argv[1]);
	string tracker_ip_address, tracker_port_no;
	inFile >> tracker_ip_address >> tracker_port_no;
	
	// Creating socket file descriptor 
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{ 
		perror("socket failed"); 
		exit(EXIT_FAILURE); 
	} 
	else
	{
		cout << "[+]Tracker Socket created successfully" << endl;
	}
	
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,&opt, sizeof(opt)))
	{ 
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	else
	{
		cout << "[+]Tracker Socket attached to port: " << tracker_port_no << " successfully" << endl;
	}

	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	// inet_aton(tracker_ip.c_str(), &address.sin_addr);
	address.sin_port = htons(stoi(tracker_port_no)); 
	
	// Forcefully Binding socket to the given port 
	if (bind(server_fd, (struct sockaddr *)&address,sizeof(address))<0)
	{ 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	}
	else
	{
		cout << "[+]Tracker Socket binding to port: " << tracker_port_no << " successful" << endl;
	}

	if (listen(server_fd, MAX_CLIENTS) < 0)
	{ 
		perror("listen");
		exit(EXIT_FAILURE);
	}

	cout<<"Serving at "<<tracker_ip_address<<":"<<tracker_port_no<<endl;

	bool isquit = false;
	thread to_exit(quit_fn, ref(isquit));

	vector<thread> peer_thread;
	while(true)
	{
		if ((new_socket = accept(server_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen))<0)
		{
			perror("accept");
			exit(EXIT_FAILURE); 
		}
		cout << "New_socket Created : " << new_socket << endl;
		peer_thread.push_back(thread(handle_peers, new_socket));
	}
	cout << "peers_thread size = " << peer_thread.size() << endl;
	for(int i=0; i<peer_thread.size(); i++)
	{
		peer_thread[i].join();
	}
	return 0; 
} 
