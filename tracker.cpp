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
using namespace std;
#define TRACKER_PORT 9002
#define TRACKER_IP "127.0.0.1"
struct file
{
    string filepath;
    string fileName;
    string fileSize;
    string uid;
    string sha;
    string gid;
};
struct peer
{
    string UID;
    map<string, int> groupId; // <gid, isOwner>  0/1
    bool isloggedIn;
    string password;
    string peer_IP;
    int peer_PORT;
    vector<string> uploadfiles;
    vector<string> download_files;
};

map<string, vector<pair<string, string>>> clients_having_file; // storing <hashvalue, <UID,gid>
map<string, peer> peer_details;                                // storing <uid, struct peer>
vector<string> tokens;                                         //storing <client input tokens>
map<int, string> port_user;                                    // storing <port, userid>
map<string, string> groupId_owner;                             // storing <group_id, user_id>
map<string, vector<string>> groupId_users;                     // storing <groupid,<users>>
map<string, vector<file>> group_file_details;                  //storing <groupid,struct obj>

void *quit_tracker(void *arg)
{
    string quit;
    cin >> quit;
    if (quit == "quit")
        exit(0);
    return NULL;
}

void *peer_parser(void *arg)
{
    cout << "Connection Request Accepted" << endl;
    struct peer peer_obj;
    int client_socket = *(int *)arg;
    char client_request[256];
    bzero(client_request, 256);
    
    recv(client_socket, &client_request, sizeof(client_request), 0);
    cout << "client Request: " << client_request << endl;
    char s = client_request[0];
    if (s == 'a')
    { //a->create_user
        //I may change here the token getting thing from line 47 to 54. In every code req.
        string cr_acc = client_request;
        cr_acc = cr_acc.substr(2); //ip:port:user:pass
        // cout<<cr_acc<<"\n";

        stringstream check1(cr_acc);
        string intermediate;
        while (getline(check1, intermediate, ':'))
            tokens.push_back(intermediate);
        // for(int i = 0; i < tokens.size(); i++)
        // 	cout << tokens[i] << '\n';
        string peer_ip = tokens[0];
        int port = stoi(tokens[1]);
        string uid = tokens[2];
        string pass = tokens[3];
        //create_acc(peer_ip, port, uid, pass, client_socket);
        peer_obj.UID = uid;
        peer_obj.password = pass;
        peer_obj.peer_IP = peer_ip;
        peer_obj.peer_PORT = port;
        peer_obj.isloggedIn = false;
        // p_arr.push_back(peer_obj);
        if (peer_details.find(uid) == peer_details.end())
        {
            peer_details[uid] = peer_obj;
            port_user[port] = uid;
            char tracker_response[256] = "Account creation successful!!";
            //write(client_socket, tracker_response, sizeof(tracker_response));
            send(client_socket, tracker_response, sizeof(tracker_response), 0);
        }
        else
        {
            char tracker_response[256] = "UserId already exist!!";
            send(client_socket, tracker_response, sizeof(tracker_response), 0);
        }
        tokens.clear();
    }

    else if (s == 'b')
    { // for login
        string cr_acc = client_request;
        cr_acc = cr_acc.substr(2); 
        // cout<<cr_acc<<"\n";
        stringstream check1(cr_acc);
        string intermediate;
        while (getline(check1, intermediate, ':'))
            tokens.push_back(intermediate);

       
        string peer_ip = tokens[0];
        int port = stoi(tokens[1]);
        string uid = tokens[2];
        string pass = tokens[3];

        if (peer_details.find(uid) != peer_details.end())
        {
            if (peer_details[uid].password == pass)
            {
                if (peer_details[uid].isloggedIn == false)
                {
                    peer_details[uid].isloggedIn = true;
                    char tracker_response[256] = "Login successful!!";
                    send(client_socket, tracker_response, sizeof(tracker_response), 0);
                }
                else
                {
                    char tracker_response[256] = "Already logged in!!";
                    send(client_socket, tracker_response, sizeof(tracker_response), 0);
                }
            }
            else
            {
                char tracker_response[256] = "Pwd failed!!";
                send(client_socket, tracker_response, sizeof(tracker_response), 0);
            }
        }
        else
        {
            char tracker_response[256] = "UserId does not exist!!";
            send(client_socket, tracker_response, sizeof(tracker_response), 0);
        }
        tokens.clear();
    }

    else if (s == 'c')
    { // for create_group
        string cr_acc = client_request;
        cr_acc = cr_acc.substr(2); //ip:port:user:pass
        // cout<<cr_acc<<"\n";
        stringstream check1(cr_acc);
        string intermediate;
        while (getline(check1, intermediate, ':'))
            tokens.push_back(intermediate);

        // for(int i = 0; i < tokens.size(); i++)
        // 	cout << tokens[i] << '\n';
        string peer_ip = tokens[0];
        int port = stoi(tokens[1]);
        string gid = tokens[2];

        string uid = port_user[port];

        if (peer_details.find(uid) != peer_details.end())
        {
            if (peer_details[uid].isloggedIn == true)
            {
                if (groupId_owner.find(gid) == groupId_owner.end())
                {
                    groupId_owner[gid] = uid;
                    peer_details[uid].groupId[gid] = 1;
                    groupId_users[gid].push_back(uid);
                    char tracker_response[256] = "Group Creation successful!!";
                    send(client_socket, tracker_response, sizeof(tracker_response), 0);
                }
                else
                {
                    char tracker_response[256] = "GroupId already exist!!";
                    send(client_socket, tracker_response, sizeof(tracker_response), 0);
                }
            }
            else
            {
                char tracker_response[256] = "Login to create group!!";
                send(client_socket, tracker_response, sizeof(tracker_response), 0);
            }
        }
        else
        {
            char tracker_response[256] = "UserId does not exist!!";
            send(client_socket, tracker_response, sizeof(tracker_response), 0);
        }
        tokens.clear();
    }

    else if (s == 'd')
    { // for logout
        string cr_acc = client_request;
        cr_acc = cr_acc.substr(2); //ip:port
        // cout<<cr_acc<<"\n";
        stringstream check1(cr_acc);
        string intermediate;
        while (getline(check1, intermediate, ':'))
            tokens.push_back(intermediate);

        // for(int i = 0; i < tokens.size(); i++)
        // 	cout << tokens[i] << '\n';
        string peer_ip = tokens[0];
        int port = stoi(tokens[1]);

        string uid = port_user[port];

        if (peer_details.find(uid) != peer_details.end())
        {
            if (peer_details[uid].isloggedIn == true)
            {
                peer_details[uid].isloggedIn = false;
                char tracker_response[256] = "Logged Out successful!!";
                send(client_socket, tracker_response, sizeof(tracker_response), 0);
            }
            else
            {
                char tracker_response[256] = "Already Looged Out!!";
                send(client_socket, tracker_response, sizeof(tracker_response), 0);
            }
        }
        else
        {
            char tracker_response[256] = "UserId does not exist!!";
            send(client_socket, tracker_response, sizeof(tracker_response), 0);
        }
        tokens.clear();
    }

    else if (s == 'e')
    { // for joining goup
        string cr_acc = client_request;
        cr_acc = cr_acc.substr(2); //ip:port:user:pass
        // cout<<cr_acc<<"\n";
        stringstream check1(cr_acc);
        string intermediate;
        while (getline(check1, intermediate, ':'))
            tokens.push_back(intermediate);

        // for(int i = 0; i < tokens.size(); i++)
        // 	cout << tokens[i] << '\n';
        string peer_ip = tokens[0];
        int port = stoi(tokens[1]);
        string gid = tokens[2];

        string uid = port_user[port];

        if (peer_details.find(uid) != peer_details.end())
        {
            if (peer_details[uid].isloggedIn == true)
            {
                if (groupId_owner.find(gid) != groupId_owner.end())
                {
                    string owner_id = groupId_owner[gid];
                    //TODO if uid == owner id .....u r creater of the group.................
                    string owner_ip = peer_details[owner_id].peer_IP;
                    int owner_port = peer_details[owner_id].peer_PORT;
                    string msg = uid + ":" + owner_ip + ":" + to_string(owner_port) + "\0";
                    char tracker_response[256];
                    // = msg.c_str	();
                    strcpy(tracker_response, msg.c_str());
                    send(client_socket, tracker_response, sizeof(tracker_response), 0);
                }
                else
                {
                    char tracker_response[256] = "GroupId does not exist!!";
                    send(client_socket, tracker_response, sizeof(tracker_response), 0);
                }
            }
            else
            {
                char tracker_response[256] = "Login to join group!!";
                send(client_socket, tracker_response, sizeof(tracker_response), 0);
            }
        }
        else
        {
            char tracker_response[256] = "UserId does not exist!!";
            send(client_socket, tracker_response, sizeof(tracker_response), 0);
        }
        tokens.clear();
    }

    else if (s == 'f')
    { // for listing groups
        string cr_acc = client_request;
        cr_acc = cr_acc.substr(2); //ip:port:user:pass
        // cout<<cr_acc<<"\n";
        stringstream check1(cr_acc);
        string intermediate;
        while (getline(check1, intermediate, ':'))
            tokens.push_back(intermediate);

        // for(int i = 0; i < tokens.size(); i++)
        // 	cout << tokens[i] << '\n';
        string peer_ip = tokens[0];
        int port = stoi(tokens[1]);

        string uid = port_user[port];
        string msg = "";
        if (peer_details.find(uid) != peer_details.end())
        {
            if (peer_details[uid].isloggedIn == true)
            {
                // int grp_size =
                if (groupId_owner.size() == 0)
                    cout << "No groups exist\n";

                else
                {
                    for (auto i : groupId_owner)
                        msg += i.first + "\n";
                }
                // char tracker_response[256] = "Login first!!";
                send(client_socket, msg.c_str(), sizeof(msg), 0);
            }
            else
            {
                char tracker_response[256] = "Login first!!";
                send(client_socket, tracker_response, sizeof(tracker_response), 0);
            }
        }
        else
        {
            char tracker_response[256] = "UserId does not exist!!";
            send(client_socket, tracker_response, sizeof(tracker_response), 0);
        }
        tokens.clear();
    }

    else if (s == 'g')
    { // for accepting requests
        string acc_req = client_request;
        acc_req = acc_req.substr(2); //ip:port:user:pass
        // cout<<cr_acc<<"\n";
        stringstream check1(acc_req);
        string intermediate;
        while (getline(check1, intermediate, ':'))
            tokens.push_back(intermediate);

        // for(int i = 0; i < tokens.size(); i++)
        // 	cout << tokens[i] << '\n';
        string peer_ip = tokens[0];
        int port = stoi(tokens[1]);
        string req_uid = tokens[2]; // accept request for this uid
        string gid = tokens[3];
        string uid = port_user[port];

        if (peer_details.find(uid) != peer_details.end())
        {
            if (peer_details[uid].isloggedIn == true)
            {
                if (groupId_owner.find(gid) != groupId_owner.end())
                {
                    groupId_users[gid].push_back(req_uid);
                    peer_details[req_uid].groupId[gid] = 0;
                    char tracker_response[256] = "Group request accepted!!";
                    send(client_socket, tracker_response, sizeof(tracker_response), 0);
                }
                else
                {
                    char tracker_response[256] = "GroupId does not exist!!";
                    send(client_socket, tracker_response, sizeof(tracker_response), 0);
                }
            }
            else
            {
                char tracker_response[256] = "Login to join group!!";
                send(client_socket, tracker_response, sizeof(tracker_response), 0);
            }
        }
        else
        {
            char tracker_response[256] = "UserId does not exist!!";
            send(client_socket, tracker_response, sizeof(tracker_response), 0);
        }
        tokens.clear();
    }

    //--------------------------------------------------------------------------------------
    else if (s == 'h')
    { // for leaving goup
        string leave_request = client_request;
        leave_request = leave_request.substr(2); //ip:port:gid
        // cout<<cr_acc<<"\n";
        stringstream check1(leave_request);
        string intermediate;
        while (getline(check1, intermediate, ':'))
            tokens.push_back(intermediate);

        // for(int i = 0; i < tokens.size(); i++)
        // 	cout << tokens[i] << '\n';
        string peer_ip = tokens[0];
        int port = stoi(tokens[1]);
        string gid = tokens[2];

        string uid = port_user[port];
        // string uid_of_group_owner = groupId_owner[gid];
        if (peer_details.find(uid) != peer_details.end())
        {
            if (peer_details[uid].isloggedIn == true)
            {
                if (groupId_owner.find(gid) != groupId_owner.end())
                {
                    string owner_id = groupId_owner[gid];
                    if (owner_id == uid)
                    {
                        cout << "uid is owner\n";
                        groupId_owner.erase(gid);
                        groupId_users.erase(gid);

                        for (auto i : peer_details)
                        {
                            if (((i.second).groupId).find(gid) != ((i.second).groupId).end())
                            {
                                cout << i.first << endl;
                                (i.second).groupId.erase(gid);
                            }
                        }
                    }
                    else
                    {
                        bool flag = false;
                        int count = 0;
                        for (auto i : groupId_users[gid])
                        {
                            if (i == uid)
                            {
                                flag = true;
                                break;
                            }
                            count++;
                        }
                        if (flag == true)
                        {
                            groupId_users[gid].erase(groupId_users[gid].begin() + count);
                        }
                        else
                        {
                            char tracker_response[256] = "User does not belong to this groupId!!\n";
                            send(client_socket, tracker_response, sizeof(tracker_response), 0);
                            ;
                            tokens.clear();
                            return NULL;
                        }

                        peer_details[uid].groupId.erase(gid);
                    }
                    // string owner_ip = peer_details[owner_id].cip;
                    // int owner_port = peer_details[owner_id].cport;
                    // string msg = uid + ":" + owner_ip + ":" + to_string(owner_port)+"\0";

                    char tracker_response[256] = "U have left the group\n";
                    // = msg.c_str	();
                    send(client_socket, tracker_response, sizeof(tracker_response), 0);
                    
                }
                else
                {
                    char tracker_response[256] = "GroupId does not exist!!";
                    send(client_socket, tracker_response, sizeof(tracker_response), 0);
                    
                }
            }
            else
            {
                char tracker_response[256] = "Login to leave group!!";
                send(client_socket, tracker_response, sizeof(tracker_response), 0);
                
            }
        }
        else
        {
            char tracker_response[256] = "UserId does not exist!!";
            send(client_socket, tracker_response, sizeof(tracker_response), 0);
            
        }
        tokens.clear();
    }

    else if (s == 'i')
    { // for uploading file
        string upload_request = client_request;
        upload_request = upload_request.substr(2); //ip:port:filepath:gid
        // cout<<cr_acc<<"\n";
        stringstream check1(upload_request);
        string intermediate;
        while (getline(check1, intermediate, ':'))
            tokens.push_back(intermediate);

        for(int i = 0; i < tokens.size(); i++)
        	cout << tokens[i] << '\n';
        string peer_ip = tokens[0];
        int port = stoi(tokens[1]);
        string filepath = tokens[2];
        string gid = tokens[3];
        string uid = port_user[port];
        string inFile_size = tokens[4];
        string sha = tokens[5];

        string filename = "";
        for (int i = filepath.length() - 1; i >= 0; i--)
        {
            if (filepath[i] != '/')
            {
                filename = filepath[i] + filename;
            }
            else
            {
                break;
            }
        }
        // handle when repeated upload by same uid same file

        // cout << "size of vector : " << groupId_users[gid].size() << endl;

        if (peer_details.find(uid) != peer_details.end())
        {
            if (peer_details[uid].isloggedIn == true)
            {
                if (groupId_owner.find(gid) != groupId_owner.end())
                {
                    if (find(groupId_users[gid].begin(), groupId_users[gid].end(), uid) != groupId_users[gid].end())
                    {

                        struct file grp_file_obj;
                        grp_file_obj.filepath = filepath;
                        grp_file_obj.fileSize = inFile_size;
                        grp_file_obj.gid = gid;
                        grp_file_obj.uid = uid;
                        grp_file_obj.fileName = filename;
                        grp_file_obj.sha = sha;
                        group_file_details[gid].push_back(grp_file_obj);
                        // file_hash_size[filename] = {sha, inFile_size};
                        // if(find(clients_having_file[sha].begin(), clients_having_file[sha].end(), {uid, gid}) == clients_having_file[sha].end()){
                        bool flag = false;
                        for (auto i : clients_having_file[sha])
                        {
                            if (i.first == uid)
                            {
                                char tracker_response[256] = "file already uploaded by this uid\n";
                                send(client_socket, tracker_response, sizeof(tracker_response), 0);
                                flag = true;
                                break;
                            }
                        }
                        if (flag == false)
                        {
                            char tracker_response[256] = "file adding to client list\n";
                            clients_having_file[sha].push_back({uid, gid});
                            send(client_socket, tracker_response, sizeof(tracker_response), 0);
                            
                        }
                        // char tracker_response[256] = "File Uploaded successful!!";
                    }
                    else
                    {
                        char tracker_response[256] = "You are not part of this group!!";
                        send(client_socket, tracker_response, sizeof(tracker_response), 0);
                        
                    }
                }
                else
                {
                    char tracker_response[256] = "GroupId does not exist!!";
                    send(client_socket, tracker_response, sizeof(tracker_response), 0);
                    
                }
            }
            else
            {
                char tracker_response[256] = "Login to upload file!!";
                send(client_socket, tracker_response, sizeof(tracker_response), 0);
                
            }
        }
        else
        {
            char tracker_response[256] = "UserId does not exist!!";
            send(client_socket, tracker_response, sizeof(tracker_response), 0);
            
        }
        tokens.clear();
    }

    else if (s == 'j')
    { // for listing file in a group
        string list_file_request = client_request;
        list_file_request = list_file_request.substr(2); //ip:port:filepath:gid
        // cout<<cr_acc<<"\n";
        stringstream check1(list_file_request);
        string intermediate;
        while (getline(check1, intermediate, ':'))
            tokens.push_back(intermediate);

        string peer_ip = tokens[0];
        int port = stoi(tokens[1]);
        string gid = tokens[2];
        string uid = port_user[port];

        string fileList = "";
        if (peer_details.find(uid) != peer_details.end())
        {
            if (peer_details[uid].isloggedIn == true)
            {
                if (groupId_owner.find(gid) != groupId_owner.end())
                {
                    set<string> fileset;
                    for (auto i : group_file_details[gid])
                        // fileList += i.fileName +"\n";
                        fileset.insert(i.fileName);

                    for (auto i : fileset)
                    {
                        fileList += (i + "\n");
                    }
                    // char tracker_response[256] = fileList;
                    write(client_socket, fileList.c_str(), sizeof(fileList));
                }
                else
                {
                    char tracker_response[256] = "GroupId does not exist!!";
                    send(client_socket, tracker_response, sizeof(tracker_response), 0);
                    
                }
            }
            else
            {
                char tracker_response[256] = "Login to get List of files!!";
                send(client_socket, tracker_response, sizeof(tracker_response), 0);
                
            }
        }
        else
        {
            char tracker_response[256] = "UserId does not exist!!";
            send(client_socket, tracker_response, sizeof(tracker_response), 0);
            
        }
        tokens.clear();
    }

    else if (s == 'k')
    { // for downloading file
        string download_request = client_request;
        download_request = download_request.substr(2); //ip:port:user:pass
        // cout<<cr_acc<<"\n";
        stringstream check1(download_request);
        string intermediate;
        while (getline(check1, intermediate, ':'))
            tokens.push_back(intermediate);

        // for(int i = 0; i < tokens.size(); i++)
        // 	cout << tokens[i] << '\n';
        string peer_ip = tokens[0];
        int port = stoi(tokens[1]);
        string gid = tokens[2];
        string filename = tokens[3];
        string sha, filesize;
        string filepath;
        if (group_file_details.find(gid) != group_file_details.end())
        {
            for (auto i : group_file_details[gid])
            {
                if (i.fileName == filename)
                {
                    sha = i.sha;
                    filesize = i.fileSize;
                    filepath = i.filepath;
                }
            }
        }

        string uid = port_user[port];
        string download_response;
        if (peer_details.find(uid) != peer_details.end())
        {
            if (peer_details[uid].isloggedIn == true)
            {
                if (groupId_owner.find(gid) != groupId_owner.end())
                {
                    if (find(groupId_users[gid].begin(), groupId_users[gid].end(), uid) != groupId_users[gid].end())
                    {
                        for (auto i : clients_having_file[sha])
                        {
                            if(i.first != uid)
                            {
                                if (peer_details[i.first].isloggedIn)
                                    download_response += (to_string(peer_details[i.first].peer_PORT) + ":");
                            }
                        }

                        download_response += (sha + ":" + filesize + ":" + filepath);
                        char tracker_response[256];

                        strcpy(tracker_response, download_response.c_str());
                        send(client_socket, tracker_response, sizeof(tracker_response), 0);
                        
                    }
                    else
                    {
                        char tracker_response[256] = "You are not part of this group!!";
                        send(client_socket, tracker_response, sizeof(tracker_response), 0);
                        
                    }
                }
                else
                {
                    char tracker_response[256] = "GroupId does not exist!!";
                    send(client_socket, tracker_response, sizeof(tracker_response), 0);
                    
                }
            }
            else
            {
                char tracker_response[256] = "Login to join group!!";
                send(client_socket, tracker_response, sizeof(tracker_response), 0);
                
            }
        }
        else
        {
            char tracker_response[256] = "UserId does not exist!!";
            send(client_socket, tracker_response, sizeof(tracker_response), 0);
            
        }
        tokens.clear();
    }

    else if (s == 'l')
    { // for stop sharing
        string stop_sharing_request = client_request;
        stop_sharing_request = stop_sharing_request.substr(2);

        stringstream check1(stop_sharing_request);
        string intermediate;
        while (getline(check1, intermediate, ':'))
            tokens.push_back(intermediate); //

        // for(int i = 0; i < tokens.size(); i++)
        // 	cout << tokens[i] << '\n';
        string peer_ip = tokens[0];
        int port = stoi(tokens[1]);
        string gid = tokens[2];
        string filename = tokens[3];
        string sha;

        if (group_file_details.find(gid) != group_file_details.end())
        {
            for (auto i : group_file_details[gid])
            {
                if (i.fileName == filename)
                {
                    sha = i.sha;
                }
            }
        }

        string uid = port_user[port];
        string stop_sharing_response;
        if (peer_details.find(uid) != peer_details.end())
        {
            if (peer_details[uid].isloggedIn == true)
            {
                if (groupId_owner.find(gid) != groupId_owner.end())
                {
                    if (find(groupId_users[gid].begin(), groupId_users[gid].end(), uid) != groupId_users[gid].end())
                    {
                        int count = 0, flag = 0;
                        for (auto i : clients_having_file[sha])
                        {
                            if (i.first == uid)
                            {
                                clients_having_file[sha].erase(clients_having_file[sha].begin() + count);
                                cout << "You have deleted your sharing from tracker" << endl;
                                stop_sharing_response = "s";
                                flag = 1;
                                break;
                            }
                            count++;
                        }
                        if (flag == 0)
                        {
                            cout << "You don't have the file to stop share" << endl;
                            stop_sharing_response = "f";
                        }

                        char tracker_response[256];
                        strcpy(tracker_response, stop_sharing_response.c_str());
                        send(client_socket, tracker_response, sizeof(tracker_response), 0);
                        
                    }
                    else
                    {
                        char tracker_response[256] = "You are not part of this group!!";
                        send(client_socket, tracker_response, sizeof(tracker_response), 0);
                        
                    }
                }
                else
                {
                    char tracker_response[256] = "GroupId does not exist!!";
                    send(client_socket, tracker_response, sizeof(tracker_response), 0);
                    
                }
            }
            else
            {
                char tracker_response[256] = "Login to join group!!";
                send(client_socket, tracker_response, sizeof(tracker_response), 0);
                
            }
        }
        else
        {
            char tracker_response[256] = "UserId does not exist!!";
            send(client_socket, tracker_response, sizeof(tracker_response), 0);
            
        }
        tokens.clear();
    }

    return NULL;
}

int main(int argc, char const *argv[])
{

    cout << "This is tracker" << endl;
    int port_no = TRACKER_PORT; //PORT defined here
    //Setting
    pthread_t tid1;
    pthread_create(&tid1, NULL, quit_tracker, NULL);
    string tracker_no = argv[2];
    string tracker_filename = argv[1];
    string tracker_details = tracker_no + " " + TRACKER_IP + ":" + to_string(TRACKER_PORT);
    ofstream file(tracker_filename);
    file << tracker_details;
    file.close();

    int server_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_no);
    server_address.sin_addr.s_addr = INADDR_ANY;

    int bind_status = bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));

    listen(server_socket, 5);

    cout << "Server is LISTENING................" << endl;

    struct sockaddr_in client_address;
    pthread_t thread_id;

    int client_socket;
    int len_client = sizeof(client_address);
    while ((client_socket = accept(server_socket, (sockaddr *)&client_address, (socklen_t *)(&len_client))) >= 0)
    {
        cout << "*****New Connection Received from " << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port) << "*****" << endl;
        //cout<<"*****New Connection Received*****"<<endl;
        //client_socket = accept(server_socket,NULL,NULL);
        pthread_create(&thread_id, NULL, peer_parser, (void *)&client_socket);
        //pthread_join(thread_id,NULL);
    }

    shutdown(server_socket, SHUT_RDWR);
    return 0;
}
