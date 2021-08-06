# Peer-To-Peer-File-Sharing-System
Peer-to-Peer application mimics torrent but with limited features. This is a group-based file sharing system where users can share, download files from the group they belong to. 


## Steps to run Peer-to-Peer File Sharing System

- download the zip file of the code.
- compile the tracker using the following commnad - 
-       g++ tracker.cpp -o tracker -lcrypto -lpthread
- run the tracker using following command -
-      ./tracker tracker_info.txt 1
- compile the peer using the following command - 
-       g++ peer.cpp -o peer -lcrypto -lpthread
- run the peer using following command-
-       ./peer ip_address:port_number

- Now register yourself as a peer using following command-
-       create_user <user_name> <password>
- Now login with your credentials using the following command-
-       login <user_name> <password>
- Now either create a new group or join a pre-existing group
- To create a new group use the following command-
-       create_group <group_id>
- Or to join pre-existing group first fetch the list of all the available group using the following command-
-       list_groups
-   now select the group id of the group you want to join and join that group using the following command-
-       join_group <group_id>
- once the join_group command is executed then the request will be sent to creater of the group.
- After receving the request admin of the group wil check the available request using the following command and this command returns the user_name_of_requesting_peer
-       list_requests <group_id>
-   To accept any request from all the available request use the following command-
-       accept_request <group_id> <user_name_of_requesting_peer>
-  To upload file in a group use the following command-
-       upload_file <file_path> <group_id>
-  To get the list of all available files in a group
-       list_files <group_id>
-   To download file from a group-
-       download_file <group_id> <file_name> <destination_path>
-   To see the status of dowload whether it is completed or not use the following command
-       show_downloads
-  To stop sharing a file use following command-
-       stop_share <group_id> <file_name>
-  To  leave a group use the following command -
-       leave_group <group_id>
- To quit the tracker use the follwoing command-
-     quit
