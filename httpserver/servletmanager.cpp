#include "servletmanager.h"

ServletManager* ServletManager::manager = NULL;

ServletManager::ServletManager(QObject *parent) : QObject(parent) {
}

void ServletManager::setUp(QObject *parent) {
    if (manager == NULL) {
        manager = new ServletManager(parent);
    }
}

ServletManager* ServletManager::getInstance() {
    return manager;
}

// Todo deconstructor: 
// cleaenup memory
ServletManager::~ServletManager() {

}

bool ServletManager::setMethod(QString path, Servlet* servlet) {
	if (MethodMap.contains(path))
		return false;
	else {
		// do a deep copy of the object
		MethodMap.insert(path, servlet);
		return true;
	}
}

Servlet* ServletManager::getMethod(QString path) {
	return MethodMap.value(path);
}


void ServletManager::PutSession(QString name, HttpSession* session) {
    session->setMaxInterval(3600); // 3600 seconds
    sessions.insert(name, session);
}

void ServletManager::deleteSession(QString sessionID) {
    HttpSession* session = sessions.value(sessionID);
    sessions.remove(sessionID);
    delete(session);
}

HttpSession* ServletManager::HttpGetSession(QString name) {
    HttpSession* re;
 
    if (sessions.contains(name)) {
        re = sessions.value(name);
        re->setIsNew(false);
        re->setLastAccessTime();

        if (!re->isValid()) {
            sessions.remove(name);

            re->setInvalid();
        } else {
            sessions.insert(name, re);
        }
        return re;
 } else  {
        return NULL;
 }
}

void ServletManager::updateIpMap(int key, std::vector<string> ipList) {
	ipMap[key] = ipList; 
}

std::vector<string> ServletManager::getIpList(int key) {
	if (ipMap.find(key) == ipMap.end()) {
		return std::vector<string>();
	}
	return ipMap[key];
}

string ServletManager::getRandomBackendIp(string username) {

    int num = (username[0] - 'a') % 4;
    std::vector<string> ipList = getIpList(num);
    if (ipList.empty()) {
    	BigTableClient master_client(grpc::CreateChannel("localhost:5000",
                      grpc::InsecureChannelCredentials()));
        manager->updateIpMap(num, master_client.FindIpList(username));
        ipList = getIpList(num);
    }
    //shuffle here !!
    for (int i = 0; i < ipList.size(); i++) {
        if (heartbeat(ipList[i])) {
        	return ipList[i];
        }
    }
    return NULL; 

}

BigTableClient ServletManager::getBigTableClient(string username) {

	string backendAddr = getRandomBackendIp(username);
	BigTableClient bigtable (grpc::CreateChannel(backendAddr,
                      grpc::InsecureChannelCredentials()));
	return bigtable;
}


bool ServletManager::heartbeat(string ip_addr) {
    size_t found = ip_addr.find_first_of(':');
    string port_num = ip_addr.substr(found + 1);
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("Cannot open socket (%s)\n", strerror(errno));
        exit(1);
    }
    // connect to the server
    struct sockaddr_in dest;
    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(stoi(port_num));
    inet_pton(AF_INET, ip_addr.c_str(), &(dest.sin_addr));
    if (::connect(sock, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        printf("server %s is dead\n", port_num.c_str());
        return false;
    } 
    printf("server %s is alive\n", port_num.c_str());
    return true;
}
// void ServletManager::addIpAddress(QString username, QString ipAddress) {
// 	QStringList* current = NULL;
//     if (userIpMap.contains(username)) {
//         current = userIpMap.value(username);
// 	} else {
// 		current = new QStringList();
//         userIpMap.insert(username, current);
// 	}

// 	(*current) << ipAddress;
// }

// QStringList* ServletManager::getIpAddress(QString username) {
// 	if (userIpMap.contains(username)) {
//         QStringList* currentlist = userIpMap.value(username);
//         return currentlist;
// 	} else {
// 		return NULL;
// 	}
// }


