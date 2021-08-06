#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <iostream>
#include <bits/stdc++.h>
#include <openssl/sha.h>
#include <ctype.h>
#include <string>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
using namespace std;

#define TRACKER_PORT 9002
#define CHUNK_SIZE 512
#define CLIENT_IP "127.0.0.1"
#define MAX_MSG_LEN 8192

map<string, vector<string>> files_completed;   // gid:filename
map<string, vector<string>> files_downloading; // gid:filename
map<string, vector<string>> join_requests;     // (gid, <users>)
vector<string> tokens;

void *other_peer_parser(void *arg)
{
    int server_socket = *(int *)arg;
    char buff_req[MAX_MSG_LEN];
    int n;
    bzero(buff_req, MAX_MSG_LEN);
    recv(server_socket, buff_req, sizeof(buff_req), 0);
    char s = buff_req[0];
    if (s == 'a')
    { //a->join user
        string j_req = buff_req;
        j_req = j_req.substr(2); 
        //<MAY> CHANGE THIS
        stringstream check1(j_req);
        string intermediate;
        while (getline(check1, intermediate, ':'))
            tokens.push_back(intermediate);

        string uid = tokens[0]; // uid of requester
        string gid = tokens[1];

        join_requests[gid].push_back(uid);
        char buff_response[MAX_MSG_LEN] = "Request for joining has been received\n";
        send(server_socket, buff_response, sizeof(buff_response), 0);
        tokens.clear();
    }
    if (s == 'z')
	{ 
        //z->download_file_request
		string j_req = buff_req;
		j_req = j_req.substr(2); //
		tokens.clear();
		stringstream check1(j_req);
		string intermediate;
		while (getline(check1, intermediate, ':'))
			tokens.push_back(intermediate);

		int chunk_no = stoi(tokens[0]); // chunk_no_requested
		string filename = tokens[1];	//filename is filepath
		char buffer[CHUNK_SIZE];		// buffer to be sent to requester
		FILE *fptr;
		fptr = fopen(filename.c_str(), "r");
		fseek(fptr, chunk_no * CHUNK_SIZE, SEEK_SET);
		int val = fread(buffer, 1, CHUNK_SIZE, fptr);
		string buff_response(buffer);
		char check_response[MAX_MSG_LEN];
		buff_response = to_string(val) + ":" + buff_response;
		strcpy(check_response, buff_response.c_str());
		cout << "Buffer Response from func = " << check_response << endl;
		//char buff_response[MAX] = "request for joining has been received\n";
		send(server_socket, check_response, sizeof(check_response), 0);
		fclose(fptr);
		tokens.clear();
	}

    return NULL;
}

void *server_thread_handler(void *arg)
{

    char *peer1 = (char *)arg;
    string peer1_address = peer1;

    int separator_pos = peer1_address.find(":");
    string ip_address = peer1_address.substr(0, separator_pos);
    string port = peer1_address.substr(separator_pos + 1);
    int port_no = stoi(port);

    //------------------------------------
    int server_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_no);
    server_address.sin_addr.s_addr = inet_addr(ip_address.c_str());
   

    int bind_status = bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));

    if (listen(server_socket, 5) < 0)
    {
        perror("Not Listening");
        exit(1);
    }
    else
    {
        cout << "Peer1 Server is LISTENING................" << endl;
    }

    struct sockaddr_in client_address;
    pthread_t peer_as_server;

    int client_socket;
    int len_client = sizeof(client_address);
    while ((client_socket = accept(server_socket, (sockaddr *)&client_address, (socklen_t *)(&len_client))) >= 0)
    {
        cout << "*****New Connection Received from " << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port) << "*****"<<endl;
        pthread_create(&peer_as_server, NULL, other_peer_parser, (void *)&client_socket);
        //pthread_join(thread_id,NULL);
    }

    shutdown(server_socket, SHUT_RDWR);
    return NULL;
}

void *client_thread_handler(void *args)
{
    int peer_socket;
    peer_socket = socket(AF_INET, SOCK_STREAM, 0);

    string input_address;
    cout << "Enter the peer you want to connect: ";
    getline(cin, input_address);
    int separator_pos = input_address.find(":");
    string ip_address = input_address.substr(0, separator_pos);
    string port = input_address.substr(separator_pos + 1);
    int port_no = stoi(port);

    struct sockaddr_in peer_address;
    peer_address.sin_family = AF_INET;
    peer_address.sin_port = htons(port_no);
    peer_address.sin_addr.s_addr = inet_addr(ip_address.c_str());

    int status = connect(peer_socket, (struct sockaddr *)&peer_address, sizeof(peer_address));
    if (status == 0)
        cout << "Connected to Peer Sever" << endl;
    else
        cout << "Peer Server Not Available" << endl;

    char request_to_peer[MAX_MSG_LEN];
    char response_from_peer[MAX_MSG_LEN];
    
    while (1)
    {
        int n = 0;
        bzero(request_to_peer, MAX_MSG_LEN);

        cout << "Enter the message to send to server: ";
        while ((request_to_peer[n++] = getchar()) != '\n')
            ;
        send(peer_socket, request_to_peer, sizeof(request_to_peer), 0);

        bzero(request_to_peer, MAX_MSG_LEN);

        recv(peer_socket, &response_from_peer, sizeof(response_from_peer), 0);

        cout << "Message Received from Server: " << response_from_peer << endl;

        if (strncmp("exit", response_from_peer, 4) == 0)
        {
            printf("Client Exit...\n");
            break;
        }
    }
    return NULL;
}
string calculate_SHA(string path, unsigned long long int sizeInBytes)
{
    char *file_buffer;
    unsigned char *result = new unsigned char[20];
    int fd = open(path.c_str(), O_RDONLY);
    file_buffer = (char *)mmap(0, sizeInBytes, PROT_READ, MAP_SHARED, fd, 0);
    SHA1((unsigned char *)file_buffer, sizeInBytes, result);
    munmap(file_buffer, sizeInBytes);
    close(fd);
    char *sha1hash = (char *)malloc(sizeof(char) * 41);
    sha1hash[41] = '\0';
    int i;
    for (i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
        sprintf(&sha1hash[i * 2], "%02x", result[i]);
    }
    // printf("SHA1 HASH: %s\n", sha1hash);
    string calculated_hash(sha1hash);
    return calculated_hash;
}

void create_account(string UID, string PASSWORD, string input_address)
{

    int separator_pos = input_address.find(":");
    string ip_address = input_address.substr(0, separator_pos);
    string port = input_address.substr(separator_pos + 1);
    int port_no = stoi(port);

    string req_to_trckr = "a:" + input_address + ":" + UID + ":" + PASSWORD + "\0";
   
    char request_to_tracker[MAX_MSG_LEN];
    strcpy(request_to_tracker, req_to_trckr.c_str());

    int server_socket;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error\n");
        return;
    }
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(TRACKER_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    char response_from_tracker[MAX_MSG_LEN] = {0};

    if (connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        printf("\nConnection Failed \n");
        return;
    }
   
    send(server_socket, request_to_tracker, sizeof(request_to_tracker), 0);
    
    bzero(response_from_tracker, MAX_MSG_LEN);
    recv(server_socket, &response_from_tracker, sizeof(response_from_tracker), 0);
    cout << response_from_tracker << endl;
    shutdown(server_socket, SHUT_RDWR);
}

void login(string user_id, string PASSWORD, string input_address)
{
    int separator_pos = input_address.find(":");
    string ip_address = input_address.substr(0, separator_pos);
    string port = input_address.substr(separator_pos + 1);
    int port_no = stoi(port);

    string req_to_trckr = "b:" + input_address + ":" + user_id + ":" + PASSWORD + "\0";
    // cout<<request_to_tracker<<"\n";
    char request_to_tracker[MAX_MSG_LEN];
    strcpy(request_to_tracker, req_to_trckr.c_str());

    int server_socket;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error\n");
        return;
    }
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(TRACKER_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    char response_from_tracker[MAX_MSG_LEN] = {0};

    if (connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        printf("\nConnection Failed \n");
        return;
    }
    
    send(server_socket, request_to_tracker, sizeof(request_to_tracker), 0);
    
    bzero(response_from_tracker, MAX_MSG_LEN);
    recv(server_socket, &response_from_tracker, sizeof(response_from_tracker), 0);
    cout << response_from_tracker << endl;
    shutdown(server_socket, SHUT_RDWR);
}

void create_group(string gid, string input_address)
{
    int separator_pos = input_address.find(":");
    string ip_address = input_address.substr(0, separator_pos);
    string port = input_address.substr(separator_pos + 1);
    int port_no = stoi(port);

    string req_to_trckr = "c:" + input_address + ":" + gid + "\0";
    // cout<<request_to_tracker<<"\n";
    char request_to_tracker[MAX_MSG_LEN];
    strcpy(request_to_tracker, req_to_trckr.c_str());

    int server_socket;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error\n");
        return;
    }
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(TRACKER_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    char response_from_tracker[MAX_MSG_LEN] = {0};

    if (connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        printf("\nConnection Failed \n");
        return;
    }
    
    send(server_socket, request_to_tracker, sizeof(request_to_tracker), 0);
    
    bzero(response_from_tracker, MAX_MSG_LEN);
    recv(server_socket, &response_from_tracker, sizeof(response_from_tracker), 0);
    cout << response_from_tracker << endl;
    shutdown(server_socket, SHUT_RDWR);
}

void logout(string input_address)
{

    int separator_pos = input_address.find(":");
    string ip_address = input_address.substr(0, separator_pos);
    string port = input_address.substr(separator_pos + 1);
    int port_no = stoi(port);

    string req_to_trckr = "d:" + input_address + "\0";
    // cout<<request_to_tracker<<"\n";
    char request_to_tracker[MAX_MSG_LEN];
    strcpy(request_to_tracker, req_to_trckr.c_str());

    int server_socket;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error\n");
        return;
    }
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(TRACKER_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    char response_from_tracker[MAX_MSG_LEN] = {0};

    if (connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        printf("\nConnection Failed \n");
        return;
    }
    
    send(server_socket, request_to_tracker, sizeof(request_to_tracker), 0);
    
    bzero(response_from_tracker, MAX_MSG_LEN);
    recv(server_socket, &response_from_tracker, sizeof(response_from_tracker), 0);
    cout << response_from_tracker << endl;
    shutdown(server_socket, SHUT_RDWR);
}

void join_group(string gid, string input_address)
{
    int separator_pos = input_address.find(":");
    string ip_address = input_address.substr(0, separator_pos);
    string port = input_address.substr(separator_pos + 1);
    int port_no = stoi(port);

    string req_to_trckr = "e:" + input_address + ":" + gid + "\0";
    // cout<<request_to_tracker<<"\n";
    char request_to_tracker[MAX_MSG_LEN];
    strcpy(request_to_tracker, req_to_trckr.c_str());

    int server_socket;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error\n");
        return;
    }
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(TRACKER_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    char response_from_tracker[MAX_MSG_LEN] = {0};

    if (connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        printf("\nConnection Failed \n");
        return;
    }
   
    send(server_socket, request_to_tracker, sizeof(request_to_tracker), 0);
 
    bzero(response_from_tracker, MAX_MSG_LEN);
    recv(server_socket, &response_from_tracker, sizeof(response_from_tracker), 0);
    cout << response_from_tracker << endl;
    string owner_address = response_from_tracker;
    separator_pos = owner_address.find(":");
    string UID = owner_address.substr(0, separator_pos);//requester uid
    string addr = owner_address.substr(separator_pos + 1);//group owner's address ip and port
    //cout << "address  : " << addr << endl;
    separator_pos = addr.find(":");
    ip_address = addr.substr(0, separator_pos);
    //cout << "my ip : " << ip_address << endl;
    port = addr.substr(separator_pos + 1);
    //cout << "port : " << port << endl;
    port_no = stoi(port);
    shutdown(server_socket, SHUT_RDWR);

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error\n");
        return;
    }
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_no);
    server_address.sin_addr.s_addr = inet_addr(ip_address.c_str());

    cout << "Connecting to ip : " << ip_address << "  port : " << port << endl;

    if (connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        printf("\nConnection Failed \n");
        return;
    }
    
    string request_message_to_owner = "a:" + UID + ":" + gid;
    send(server_socket, request_message_to_owner.c_str(), sizeof(request_message_to_owner), 0);
    string req = "request sent to owner";
    cout << req << endl;
    char response_from_owner[MAX_MSG_LEN];
    bzero(response_from_owner, MAX_MSG_LEN);
    recv(server_socket, &response_from_owner, sizeof(response_from_owner), 0);
    cout << response_from_owner << endl;
    shutdown(server_socket, SHUT_RDWR);
}

void list_groups(string input_address)
{
    int separator_pos = input_address.find(":");
    string ip_address = input_address.substr(0, separator_pos);
    string port = input_address.substr(separator_pos + 1);
    int port_no = stoi(port);

    string req_to_trckr = "f:" + input_address + "\0";
    char request_to_tracker[MAX_MSG_LEN];
    strcpy(request_to_tracker, req_to_trckr.c_str());

    int server_socket;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error\n");
        return;
    }
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(TRACKER_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    char response_from_tracker[MAX_MSG_LEN] = {0};

    if (connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        printf("\nConnection Failed \n");
        return;
    }
   
    send(server_socket, request_to_tracker, sizeof(request_to_tracker), 0);
    
    bzero(response_from_tracker, MAX_MSG_LEN);
    recv(server_socket, &response_from_tracker, sizeof(response_from_tracker), 0);
    cout << response_from_tracker << endl;
    shutdown(server_socket, SHUT_RDWR);
}

void list_requests(string gid, string input_address)
{
    vector<string> pending_users = join_requests[gid];
    for (auto i : pending_users)
    {
        cout << i << endl;
    }
}

void accept_request(string gid, string UID, string input_address)
{
    bool flag = false;
    int count = 0;
    for (auto i : join_requests[gid])
    {
        if (i == UID)
        {
            flag = true;
            join_requests[gid].erase(join_requests[gid].begin() + count);
            break;
        }
        count++;
    }
    if (flag == true)
    {
        int separator_pos = input_address.find(":");
        string ip_address = input_address.substr(0, separator_pos);
        string port = input_address.substr(separator_pos + 1);
        int port_no = stoi(port);

        string req_to_trckr = "g:" + input_address + ":" + UID + ":" + gid + "\0";
     
        char request_to_tracker[MAX_MSG_LEN];
        strcpy(request_to_tracker, req_to_trckr.c_str());

        int server_socket;
        if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("\n Socket creation error\n");
            return;
        }
        struct sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(TRACKER_PORT);
        server_address.sin_addr.s_addr = INADDR_ANY;

        char response_from_tracker[MAX_MSG_LEN] = {0};

        if (connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        {
            printf("\nConnection Failed \n");
            return;
        }
        
        send(server_socket, request_to_tracker, sizeof(request_to_tracker), 0);
        
        bzero(response_from_tracker, MAX_MSG_LEN);
        recv(server_socket, &response_from_tracker, sizeof(response_from_tracker), 0);
        cout << response_from_tracker << endl;
        shutdown(server_socket, SHUT_RDWR);
    }
}

void leave_group(string gid, string input_address)
{
    int separator_pos = input_address.find(":");
    string ip_address = input_address.substr(0, separator_pos);
    string port = input_address.substr(separator_pos + 1);
    int port_no = stoi(port);

    string req_to_trckr = "h:" + input_address + ":" + gid + "\0";
  
    char request_to_tracker[MAX_MSG_LEN];

    if (join_requests.find(gid) != join_requests.end())
    {
        cout << "deleting in join reqs \n";
        join_requests.erase(gid);
    }

  
    strcpy(request_to_tracker, req_to_trckr.c_str());
    int server_socket;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error\n");
        return;
    }
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(TRACKER_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    char response_from_tracker[MAX_MSG_LEN] = {0};

    if (connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        printf("\nConnection Failed \n");
        return;
    }
   
    send(server_socket, request_to_tracker, sizeof(request_to_tracker), 0);
   
    bzero(response_from_tracker, MAX_MSG_LEN);
    recv(server_socket, &response_from_tracker, sizeof(response_from_tracker), 0);
    cout << response_from_tracker << endl;
    shutdown(server_socket, SHUT_RDWR);
}

void upload_file(string filepath, string gid, string input_address)
{

    int separator_pos = input_address.find(":");
    string ip_address = input_address.substr(0, separator_pos);
    string port = input_address.substr(separator_pos + 1);
    int port_no = stoi(port);
    ifstream inputFile(filepath, ios::binary);
	inputFile.seekg(0, ios::end);
	int inFile_size = inputFile.tellg();
	cout << "Size of the file is"<< " " << inFile_size << " "<< "bytes" << endl;
	string sha = calculate_SHA(filepath, inFile_size);

    string req_to_trckr = "i:" + input_address + ":" + filepath + ":" + gid + ":" + to_string(inFile_size) + ":" + sha + "\0";
  
    char request_to_tracker[MAX_MSG_LEN];
  
    strcpy(request_to_tracker, req_to_trckr.c_str());
   
    int server_socket;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error\n");
        return;
    }
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(TRACKER_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    char response_from_tracker[MAX_MSG_LEN] = {0};

    if (connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        printf("\nConnection Failed \n");
        return;
    }
    
    send(server_socket, request_to_tracker, sizeof(request_to_tracker), 0);
    
    bzero(response_from_tracker, MAX_MSG_LEN);
    recv(server_socket, &response_from_tracker, sizeof(response_from_tracker), 0);
    cout << response_from_tracker << endl;
    shutdown(server_socket, SHUT_RDWR);
}

void list_files(string gid, string input_address)
{
    int separator_pos = input_address.find(":");
    string ip_address = input_address.substr(0, separator_pos);
    string port = input_address.substr(separator_pos + 1);
    int port_no = stoi(port);

    string req_to_trckr = "j:" + input_address + ":" + gid + "\0";
    
    char request_to_tracker[MAX_MSG_LEN];
    
    strcpy(request_to_tracker, req_to_trckr.c_str());
 
    int server_socket;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error\n");
        return;
    }
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(TRACKER_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    char response_from_tracker[MAX_MSG_LEN] = {0};

    if (connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        printf("\nConnection Failed \n");
        return;
    }
   
    send(server_socket, request_to_tracker, sizeof(request_to_tracker), 0);
    
    bzero(response_from_tracker, MAX_MSG_LEN);
    recv(server_socket, &response_from_tracker, sizeof(response_from_tracker), 0);
    cout << response_from_tracker << endl;
    shutdown(server_socket, SHUT_RDWR);
}



void *get_chunk_as_client(void *arg)
{
    //cout << "In get_chunk_as_client" << endl;
    char *message_to_be_sent_to_peer = (char *)arg;
    string received_message(message_to_be_sent_to_peer);
    cout << "Data received from peer client = " << received_message << endl;

  
    vector<string> tokens2;
    stringstream check1(received_message);
    string intermediate;
    while (getline(check1, intermediate, ':'))
        tokens2.push_back(intermediate);
    int tokens_size = tokens2.size();
    string ip_address = tokens2[0];
    int port = stoi(tokens2[1]);
    string chunk_no = tokens2[2];
    string filepath = tokens2[3]; 

    // //FETCH DESTINATION PATH HERE -----------------------------------------------------------------
    string destination_path = tokens2[4];

    string message = "z:" + chunk_no + ":" + filepath;
    cout << "Message sent to peerlistener server = " << message << endl;
    char message_to_peer[MAX_MSG_LEN];
    strcpy(message_to_peer, message.c_str());

    int server_socket;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error\n");
        return NULL;
    }
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr(ip_address.c_str());

    char response_from_peer[MAX_MSG_LEN] = {0};

    if (connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        printf("\nConnection Failed \n");
        return NULL;
    }
    
    send(server_socket, message_to_peer, sizeof(message), 0);
    
    bzero(response_from_peer, MAX_MSG_LEN);
    recv(server_socket, &response_from_peer, sizeof(response_from_peer), 0);
    cout << "Message to be written to file " << response_from_peer << endl;

    string response_buffer(response_from_peer);
    int pos = response_buffer.find(":");
    int val = stoi(response_buffer.substr(0, pos));
    string chunk_data = response_buffer.substr(pos + 1);
    //chunk_data+="\0";
    cout << "CHUNK data to be written to file" << chunk_data << endl;
    fstream dest_file_ptr;
    dest_file_ptr.open(destination_path, ios::out | ios::binary | ios::in);
    dest_file_ptr.seekg(stoi(chunk_no) * CHUNK_SIZE, dest_file_ptr.beg);
    dest_file_ptr.write(chunk_data.c_str(), val);
    dest_file_ptr.close();

    shutdown(server_socket, SHUT_RDWR);

    return NULL;
}

void download_file(string gid, string filename, string download_path, string input_address)
{
    int separator_pos = input_address.find(":");
    string ip_address = input_address.substr(0, separator_pos);
    string port = input_address.substr(separator_pos + 1);
    int port_no = stoi(port);
    string req_to_trckr = "k:" + input_address + ":" + gid + ":" + filename + "\0";
    cout << "Message sent to tracker = " << req_to_trckr << "\n";
    char request_to_tracker[MAX_MSG_LEN];
    // new char[msg.length()+1];
    strcpy(request_to_tracker, req_to_trckr.c_str());
    int server_socket;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error\n");
        return;
    }
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(TRACKER_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    char response_from_tracker[MAX_MSG_LEN] = {0};

    if (connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        printf("\nConnection Failed \n");
        return;
    }
   
    send(server_socket, request_to_tracker, sizeof(request_to_tracker), 0);
    
    bzero(response_from_tracker, MAX_MSG_LEN);
    recv(server_socket, &response_from_tracker, sizeof(response_from_tracker), 0);
    cout << response_from_tracker << endl;

    string peer_ports(response_from_tracker);
    // int no_of_chunks =
    tokens.clear();
    stringstream check1(peer_ports);
    string intermediate;
    while (getline(check1, intermediate, ':'))
        tokens.push_back(intermediate);

    int tokens_size = tokens.size();
    int no_of_peer_ports = tokens_size - 3;
    cout << "No. of peer ports = " << no_of_peer_ports << endl;
    if (no_of_peer_ports == 0)
    {
        cout << "No Peers Online for Downloading";
        shutdown(server_socket, SHUT_RDWR);
        return;
    }

    string sha_original_file = tokens[tokens_size - 3];
    string file_size = tokens[tokens_size - 2];
    string filepath = tokens[tokens_size - 1];
    int no_of_chunks = ceil(stof(file_size) / float(CHUNK_SIZE)); // CHANGE LATER

    //thread array for client
    pthread_t tid[no_of_chunks];

    FILE *download_file_ptr;
    download_file_ptr = fopen(download_path.c_str(), "w");
    fclose(download_file_ptr);
    //piece selection algo
    for (int i = 0; i < no_of_chunks; i++)
    {
        string colon = ":";
        string message_to_be_sent_to_peer = CLIENT_IP + colon + tokens[i % no_of_peer_ports] + colon + to_string(i) + colon + filepath + colon + download_path;
        cout << "Message to peer server = " << message_to_be_sent_to_peer << endl;
        pthread_create(&tid[i], NULL, get_chunk_as_client, (void *)message_to_be_sent_to_peer.c_str());
        pthread_join(tid[i], NULL);
        cout << "Thread " << i << " Completed " << endl;
    }
    ifstream inputFile(download_path, ios::binary);
    inputFile.seekg(0, ios::end);
    int inFile_size = inputFile.tellg();
    cout << "Size of the file is"<< " " << inFile_size << " "<< "bytes" << endl;
    string sha_downloaded_file = calculate_SHA(download_path, inFile_size);

    // cout<<"Original SHA = "<<sha_original_file<<endl;

    // cout<<"New file SHA = "<<sha_downloaded_file<<endl;

    if (strcmp(sha_original_file.c_str(), sha_downloaded_file.c_str()) == 0)
        cout << "File Downloaded Successfully!!" << endl;
    else
    {
        cout << "Error Occurred while Downloading" << endl;
    }

    shutdown(server_socket, SHUT_RDWR);
}

void show_downloads()
{

    for (auto i : files_downloading)
    {
        for (auto j : i.second)
        {
            cout << "[D]\t" << i.first << "\t" << j << endl;
        }
    }
    for (auto i : files_completed)
    {
        for (auto j : i.second)
        {
            cout << "[C]\t" << i.first << "\t" << j << endl;
        }
    }
}

void stop_sharing(string gid, string filename, string input_address)
{

    int separator_pos = input_address.find(":");
    string ip_address = input_address.substr(0, separator_pos);
    string port = input_address.substr(separator_pos + 1);
    int port_no = stoi(port);

    string req_to_trckr = "l:" + input_address + ":" + gid + ":" + filename + "\0";
    cout << "Message sent to tracker = " << req_to_trckr << "\n";
    char request_to_tracker[MAX_MSG_LEN];
    
    strcpy(request_to_tracker, req_to_trckr.c_str());
    int server_socket;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error\n");
        return;
    }
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(TRACKER_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    char response_from_tracker[MAX_MSG_LEN] = {0};

    if (connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        printf("\nConnection Failed \n");
        return;
    }
    
    send(server_socket, request_to_tracker, sizeof(request_to_tracker), 0);
    
    bzero(response_from_tracker, MAX_MSG_LEN);
    recv(server_socket, &response_from_tracker, sizeof(response_from_tracker), 0);
    cout << response_from_tracker << endl;

    if (response_from_tracker[0] == 's')
    {
        cout << "Succesfully stopped sharing " << filename << endl;
    }
    else
    {
        cout << "You dont have the file to stop sharing " << endl;
    }
    shutdown(server_socket, SHUT_RDWR);
}

int main(int argc, char const *argv[])
{
    string peer1_address = argv[1];

    cout << "This is Peer Side" << endl;

    //CREATING THREAD FOR CLIENT
    // pthread_t client_thread;
    // pthread_create(&client_thread, NULL, client_thread_handler, NULL);
    pthread_t peer_as_server;
    pthread_create(&peer_as_server, NULL, server_thread_handler, (void *)peer1_address.c_str());

    while (1)
    {

        string command;
        cin >> command;
        if (command == "create_user")
        {
            string user_id, pass;
            cin >> user_id >> pass;
            create_account(user_id, pass, peer1_address);
        }
        else if (command == "login")
        {
            string user_id, pass;
            cin >> user_id >> pass;
            login(user_id, pass, peer1_address);
            //login
        }
        else if (command == "create_group")
        {
            string gid;
            cin >> gid;
            create_group(gid, peer1_address);
            //create_group
        }
        else if (command == "logout")
        {
            logout(peer1_address);
        }
        else if (command == "join_group")
        {
            string gid;
            cin >> gid;
            join_group(gid, peer1_address);
            //join group
        }
        else if (command == "list_groups")
        {
            list_groups(peer1_address);
        }
        else if (command == "leave_group")
        {
            string gid;
            cin >> gid;
            leave_group(gid, peer1_address);
            //leave_group
        }
        else if (command == "accept_request")
		{
			string gid;
			cin >> gid;
			string user_id;
			cin >> user_id;
			accept_request(gid, user_id, peer1_address);
		}
        else if (command == "list_requests")
        {
            string gid;
            cin >> gid;
            list_requests(gid, peer1_address);
        }

        else if (command == "upload_file")
        {
            string gid, filepath;
            cin >> filepath >> gid;
            upload_file(filepath, gid, peer1_address);
        }
        
        else if (command == "list_files")
        {
            string gid;
            cin >> gid;
            list_files(gid, peer1_address);
        }
        else if (command == "download_file")
        {
            string gid, filename, dest_path;
            cin >> gid >> filename >> dest_path;
            files_downloading[gid].push_back(dest_path);
            download_file(gid, filename, dest_path, peer1_address);
            int count = 0;
            for (auto i : files_downloading[gid])
            {
                if (i == dest_path)
                {
                    files_downloading[gid].erase(files_downloading[gid].begin() + count);
                    files_completed[gid].push_back(dest_path);
                    break;
                }
                count++;
            }
            upload_file(dest_path, gid, peer1_address);
        }

        else if (command == "show_downloads")
        {
            show_downloads();
        }
        else if (command == "stop_share")
        {
            string gid, filename;
            cin >> gid >> filename;
            stop_sharing(gid, filename, peer1_address);
        }
        else
        {
            printf("!!!Invalid command!!! Re-enter\n");
        }
    }

    return 0;
}
