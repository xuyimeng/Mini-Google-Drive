#include "emailservlet.h"
#include <QString>
#include <QByteArray>
#include <QDebug>


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <map>
#include <sys/time.h>
#include <arpa/inet.h>
#include <time.h>
#include <fstream>
#include <QUrl>

#include <signal.h>
#include <vector>
#include <regex>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <resolv.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <arpa/nameser_compat.h>
#include <locale>


int debug_mode = 0;
string directory;

// find host name
string dns_lookup(string host) {
    const size_t size = 1024;
    unsigned char buffer[size]={0};
    char dispbuf[size]={0};
    ns_rr rr;
    ns_msg m;
    int r = res_query(host.c_str(), C_IN, T_MX, buffer, size);
    if (r == -1 ) {
        return "";
    }
    int k = ns_initparse (buffer, r, &m);
    if (k == -1) {
        return "";
    }
    int l = ns_msg_count(m, ns_s_an);
    ns_parserr(&m, ns_s_an, 0, &rr);
    ns_sprintrr(&m, &rr, NULL, NULL, dispbuf, sizeof(dispbuf));
    string disp;
    for (int i = 0; i < sizeof(dispbuf); i++) {
        if (dispbuf[i] != ' ') {
            disp += dispbuf[i];
        } else {
            disp = "";
        }
    }
    return disp;
}

// get host ip address 
string get_ip(string host) {
    struct hostent *he;
    struct in_addr **addr_list;
    he = gethostbyname(host.c_str());
    addr_list = (struct in_addr **)he->h_addr_list;
    string ip(inet_ntoa(*addr_list[0]));
    return ip;
}

// get host from receiver
string extract_host(string receiver) {
    for (int i = 0; i < receiver.size(); i++) {
        if (receiver[i] == '@') {
            return receiver.substr(i+1);
        }
    }
    return "";
}

// get sender address
string extract_sender(string line) {
    string sender;
    for (int i = 5; i < line.size(); i++) {
        if (line[i] == '<' || line[i] == ' ' || line[i] == '\r' || line[i] == '\n') continue;
        if (line[i] == '>') return sender;
        sender += line[i];
    }
    return sender;
}
// get receiver address list
vector<string> extract_receiver(string line){
    vector<string> res;
    string receiver;
    for (int i = 4; i < line.size(); i++) {
        if (line[i] == '<' || line[i] == '>' || line[i] == ' ' || line[i] == '\r' || line[i] == '\n') continue;
        else if (line[i] == ',') {
            res.push_back(receiver);
            receiver = "";
        } else {
            receiver += line[i];
        }
    }
    res.push_back(receiver);
    return res;
}

// send one message
void send_one(string sender, string receiver, string data, string addr) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);  
    if (sockfd < 0) {
        fprintf(stderr, "Cannot open socket (%s)\n", strerror(errno));    
        exit(1);
    }  
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(25);
    inet_pton(AF_INET, addr.c_str(), &(servaddr.sin_addr));
    // inet_pton(AF_INET, "74.125.202.27", &(servaddr.sin_addr));
    connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

    char buf [1024] = {0};

    if (read(sockfd, buf, sizeof(buf)) > 0) {
        fprintf(stderr, "%s\n", buf);
        bzero(buf, sizeof(buf));
    }
    // hello command
    string request = "HELO arg\r\n";
    if (debug_mode) fprintf(stderr, "%s\n", request.c_str());
    write(sockfd, request.c_str(), request.size());
    if (read(sockfd, buf, sizeof(buf)) > 0) {
        fprintf(stderr, "hello\n");
        fprintf(stderr, "%s\n", buf);
        bzero(buf, sizeof(buf));
    }
    // sender
    request = "Mail From:<" + sender + ">\r\n";
    if (debug_mode) fprintf(stderr, "%s\n", request.c_str());
    write(sockfd, request.c_str(), request.size());
    if (read(sockfd, buf, sizeof(buf)) > 0) {
        fprintf(stderr, "%s\n", buf);
        bzero(buf, sizeof(buf));
    }
    // receiver
    request = "RCPT To:<" + receiver + ">\r\n";
    if (debug_mode) fprintf(stderr, "%s\n", request.c_str());
    write(sockfd, request.c_str(), request.size());
    if (read(sockfd, buf, sizeof(buf)) > 0) {
        fprintf(stderr, "%s\n", buf);
        bzero(buf, sizeof(buf));
    }
    // msg data
    request = "Data\r\n";
    if (debug_mode) fprintf(stderr, "%s\n", request.c_str());
    write(sockfd, request.c_str(), request.size());
    if (read(sockfd, buf, sizeof(buf)) > 0) {
        fprintf(stderr, "%s\n", buf);
        bzero(buf, sizeof(buf));
    }
    request = data + ".\r\n";
    if (debug_mode) fprintf(stderr, "%s\n", request.c_str());
    write(sockfd, request.c_str(), request.size());
    if (read(sockfd, buf, sizeof(buf)) > 0) {
        fprintf(stderr, "%s\n", buf);
        bzero(buf, sizeof(buf));
    }
    // quit command
    request = "quit\r\n";
    if (debug_mode) fprintf(stderr, "%s\n", request.c_str());
    write(sockfd, request.c_str(), request.size());
    if (read(sockfd, buf, sizeof(buf)) > 0) {
        fprintf(stderr, "%s\n", buf);
        bzero(buf, sizeof(buf));
    }
    close(sockfd);
}
// send messages
void send_msg(string sender, string receiver, string data) {
    if (sender.empty() || receiver.empty()) return;
    
    string host = extract_host(receiver);
    string host_name = dns_lookup(host);
    string addr = get_ip(host_name);
    if (debug_mode) {
        fprintf(stderr, "sender: %s\n", sender.c_str());
        fprintf(stderr, "receiver: %s\n", receiver.c_str());
        fprintf(stderr, "host: %s\n", host.c_str());
        fprintf(stderr, "server name: %s\n", host_name.c_str());
        fprintf(stderr, "ip address: %s\n", addr.c_str());
    }
    send_one(sender, receiver, data, addr);
    
}

string getTime();


EmailServlet::EmailServlet(QObject *parent) : Servlet(parent)
{

}

void EmailServlet::doGet(HttpRequest* request, HttpResponse* response) {

    HttpSession* session = request->HttpGetSession(false);
    if(session == NULL){
        QString page = "You have not log in<br>";
        page += "<a href=\"login\">log in</a>";
        
        response->setContentType("text/html");
        response->addContent(page);
        response->setStatus(200);
        return;
    }

    QString username = session->getAttribute("username");
    response->addContent(QString("Welcome! ") +username);
    string curtUser = username.toStdString();
    
    ServletManager* manager = ServletManager::getInstance(); 

    BigTableClient bigtable = manager->getBigTableClient(curtUser);
    
	// cout << "In emailservlet doget!!!!!" <<endl;
	QString uri = request->getURI();
    qDebug() <<"!!!!!!!!In doGet";
    qDebug() << uri;

    QFile file(rootDir+"/html/email_common.html");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        qDebug() << "file could not read";
        return;
    }
           
    QByteArray page = file.readAll();
    file.close();

    if(QString::compare(uri,"/new_email",Qt::CaseSensitive) == 0){

        QFile file2(rootDir + "/html/email_new.html");
        if (!file2.open(QIODevice::ReadOnly | QIODevice::Text)){
            qDebug() << "file2 could not read";
            return;
        }
        page += file2.readAll();
        file2.close();
    }else if(QString::compare(uri,"/receive_email",Qt::CaseSensitive) == 0
            ||QString::compare(uri,"/sent_email",Qt::CaseSensitive) == 0){
        //get email receive headers from bigrable 
        
        string boxType;
        if(QString::compare(uri,"/receive_email",Qt::CaseSensitive) == 0){
            qDebug() << "in receive email!!";
            boxType = "email:receive";
        }else if(QString::compare(uri,"/sent_email",Qt::CaseSensitive) == 0){
            qDebug() << "in send email!!";
            boxType = "email:send";
        }
        string recvheaders = bigtable.GetValue(curtUser,boxType);;
       
        
        std::cout << recvheaders << std::endl;
        page += "  <div class=\"col-md-9\">\n"
                "    <table class=\"table table-condensed table-hover\">\n"
                "      <thead>\n"
                "        <tr>\n"
                "          <th class=\"span1\"></th>\n"
                "          <th class=\"span2\"><strong>Sender</strong></th>\n"
                "          <th class=\"span9\"><strong>Subject</strong></th>\n"
                "          <th class=\"span2\"><strong>Time</strong></th>\n"
                "        </tr>\n"
                "      </thead>\n"
                "    <tbody>\n";
        if(recvheaders == "$Failed$"){
            qDebug()<<"Inbox is empty now";
        }else{
            QString qrecvheaders = QString::fromStdString(recvheaders);
            // split headers of emails by /
            QStringList receiveList = qrecvheaders.split("/");
            
            for(QString receivEmail : receiveList){
                // split arguments by &
                if(receivEmail.compare("")==0){
                    continue;
                }
                QStringList headerList = receivEmail.split("&");
                QString sender;
                QString subject;
                QString time;

                for(QString h : headerList){
                    QStringList keyValue = h.split("=");
                    QString key = keyValue.at(0);
                    if(key.contains("sender", Qt::CaseSensitive)){
                        sender = keyValue.at(1);
                    }else if(QString::compare(key,"subject",Qt::CaseSensitive)==0){
                        subject = keyValue.at(1);
                    }else if(QString::compare(key,"time",Qt::CaseSensitive)==0){
                        time = keyValue.at(1);
                    }
                }
                subject.replace("+"," ");
                qDebug() << "sender:" << sender << "subject:" << subject << "time"<<time;
                QString emailquery;
                
                if(boxType.compare("email:receive") == 0 ||boxType.compare("email:send") == 0){
                    emailquery = "show_email?"+QString::fromStdString(boxType)+":"+receivEmail;
                } 
               
                page += " <tr>\n"
                        "      <td><input type=\"checkbox\"> <i class=\"icon-star-empty\"></i></td>\n"
                        "      <td><a href=\""+emailquery+"\">"+ sender +"</a></td>\n"
                        "      <td><a href=\""+emailquery+"\">"+ subject +"</a></td>\n"
                        "      <td><a href=\""+emailquery+"\">"+ time +"</a></td>\n"
                        " </tr>\n";
            }
        }
        page += "</tbody>\n"
                "</table></div></div></div>\n"
                "</body></html>\n";

    }else if(QString::compare(uri,"/trash_email",Qt::CaseSensitive) == 0){
        qDebug() << "in delete email!!";
        string recvheaders = bigtable.GetValue(curtUser,"email:trash");
        std::cout << recvheaders << std::endl;
        page += "  <div class=\"col-md-9\">\n"
                "    <table class=\"table table-condensed table-hover\">\n"
                "      <thead>\n"
                "        <tr>\n"
                "          <th class=\"span1\"></th>\n"
                "          <th class=\"span2\"><strong>Sender</strong></th>\n"
                "          <th class=\"span9\"><strong>Subject</strong></th>\n"
                "          <th class=\"span2\"><strong>Time</strong></th>\n"
                "          <th class=\"span2\"><strong>Delete</strong></th>\n"
                "        </tr>\n"
                "      </thead>\n"
                "    <tbody>\n";
        if(recvheaders == "$Failed$"){
            qDebug()<<"Inbox is empty now";
        }else{
            QString qrecvheaders = QString::fromStdString(recvheaders);
            // split headers of emails by /
            QStringList receiveList = qrecvheaders.split("/");
            
            for(QString receivEmail : receiveList){
                if(receivEmail.compare("")==0){
                    continue;
                }
                // split arguments by &
                QStringList headerList = receivEmail.split("&");
                QString sender;
                QString subject;
                QString time;

                for(QString h : headerList){
                    QStringList keyValue = h.split("=");
                    QString key = keyValue.at(0);
                    if(key.contains("sender", Qt::CaseSensitive)){
                        sender = keyValue.at(1);
                    }else if(QString::compare(key,"subject",Qt::CaseSensitive)==0){
                        subject = keyValue.at(1);
                    }else if(QString::compare(key,"time",Qt::CaseSensitive)==0){
                        time = keyValue.at(1);
                    }
                }
                subject.replace("+"," ");
                qDebug() << "sender:" << sender << "subject:" << subject << "time"<<time;
                QString emailquery;
  
                emailquery = "show_email?"+receivEmail;
                QString delete_query = "complete_delete?"+receivEmail;

                page += " <tr>\n"
                        "      <td><input type=\"checkbox\"> <i class=\"icon-star-empty\"></i></td>\n"
                        "      <td><a href=\""+emailquery+"\">"+ sender +"</a></td>\n"
                        "      <td><a href=\""+emailquery+"\">"+ subject +"</a></td>\n"
                        "      <td><a href=\""+emailquery+"\">"+ time +"</a></td>\n"
                        "      <td><a href=\""+delete_query+"\"><i class=\"fa fa-trash-o\"></i></a></td>\n"
                        " </tr>\n";
            }
        }
        
        page += "</tbody>\n"
                "</table></div></div></div>\n"
                "</body></html>\n";

    }
    else if(uri.startsWith("/show_email")){
        QString emailCol = uri.split("?").at(1);
        qDebug() << "email column : "<<emailCol;
        QString subject;
        QString sender;
        QString receiver;
        QString time;

        QStringList paramList = emailCol.split("&");
        for(QString h : paramList){
            QStringList keyValue = h.split("=");
            QString key = keyValue.at(0);
            if(key.contains("sender", Qt::CaseSensitive)){
                sender = keyValue.at(1);
            }else if(QString::compare(key,"subject",Qt::CaseSensitive)==0){
                subject = keyValue.at(1);
            }else if(QString::compare(key,"time",Qt::CaseSensitive)==0){
                time = keyValue.at(1);
            }else if(QString::compare(key,"receiver",Qt::CaseSensitive)==0){
                receiver = keyValue.at(1);
            }
        }
        subject.replace("+"," ");
        QString content = QString::fromStdString(bigtable.GetValue(curtUser,emailCol.toStdString()));
        content.replace("\n","<br>");
        
        qDebug() << "sender:" << sender << "subject:" << subject << "time"<<time << "content" << content;

        QString replyQureyStr = "email_reply?"+sender+"/"+subject;
        QString deleteQureyStr = "email_delete?"+emailCol;
        QString forwardQueryStr = "email_forward?"+emailCol;
        
        page += "  <div class=\"col-md-9\">\n"
                "    <div style=\"border-bottom: 3px solid black;background-color: lightgrey;margin-left:5px;max-width:800px;\" class=\"container\">\n"
                "       <strong>From: "+sender+"</strong><br>\n"
                "       Subject: "+subject+"<br>\n"
                "       To: "+receiver+"<br>\n"
                "       Time: "+time+"<br>\n"
                "    </div>\n";
        page += "<a href=\""+replyQureyStr+"\" class=\"btn btn-success\">Reply</a>\n"
                "<a href=\""+deleteQureyStr+"\"class=\"btn btn-danger\">Delete</a>\n"
                "<a href=\""+forwardQueryStr+"\"class=\"btn btn-primary\">Forward</a>\n"
                "<div style=\"margin-left:5px;max-width:800px;max-height:80%\">"+content+"</div>\n";   
    }else if(uri.startsWith("/email_delete")){
        QString emailCol = uri.split("?").at(1);
        QString emailCol_backup = uri.split("?").at(1);
        qDebug() << "delete email column : "<<emailCol;
        string headers_to_delete;

        bool isReceive;
        if(emailCol.startsWith("email:receive:")){
            //delete the mail from email:receive column of curtUser
            isReceive = true;
            emailCol.remove(0,14);
            headers_to_delete = bigtable.GetValue(curtUser,"email:receive"); 
        }else if(emailCol.startsWith("email:send:")){
            //delete the mail from email:send column of curtUser
            isReceive = false;
            emailCol.remove(0,11);
            headers_to_delete = bigtable.GetValue(curtUser,"email:send");
        }
        QString deleteheaders = QString::fromStdString(headers_to_delete);
        QStringList deleteList = deleteheaders.split("/");
            
        qDebug() << "orinal email:receive column:"<<deleteheaders;
        QString newHeaders = "";
        qDebug() << "*****************email col:"<<emailCol;

        for(QString receivEmail : deleteList){
           
            qDebug() << "*****************receive email:"<<receivEmail;
    
            if(QString::compare(emailCol,receivEmail,Qt::CaseInsensitive) == 0){
                qDebug() << "mached email:"<<emailCol;
                continue;
            }
            newHeaders += receivEmail + "/";
        }
        qDebug() << "************new headers"<<newHeaders;
        string newHeadersStr = newHeaders.toStdString();
        if(newHeaders.endsWith("/")){
            newHeadersStr = newHeadersStr.substr(0,newHeadersStr.size() - 1);
        }else if(newHeaders.startsWith("/")){
            newHeadersStr = newHeadersStr.substr(1,newHeadersStr.size());
        }
        
        if(isReceive){
            bigtable.PutValue(curtUser,"email:receive",newHeadersStr);
        }else{
            bigtable.PutValue(curtUser,"email:send",newHeadersStr);
        }

        /* add header to trash column 
        */
        string eamilTrashHeaders = bigtable.GetValue(curtUser,"email:trash");
        qDebug() << "email col backup"<< emailCol_backup;
       
        if(eamilTrashHeaders == "$Failed$"){
            bigtable.PutValue(curtUser,"email:trash",emailCol_backup.toStdString());
    
        }else{
            eamilTrashHeaders += "/" + emailCol_backup.toStdString();
            bigtable.PutValue(curtUser,"email:trash",eamilTrashHeaders);
        }

    }else if(uri.startsWith("/complete_delete")){
        QString emailCol = uri.split("?").at(1);
        qDebug() << "delete email column : "<<emailCol;
        bigtable.deleteValue(curtUser,emailCol.toStdString());
        string headers_to_delete = bigtable.GetValue(curtUser,"email:trash");

        QString deleteheaders = QString::fromStdString(headers_to_delete);
        QStringList deleteList = deleteheaders.split("/");
            
        qDebug() << "orinal email:receive column:"<<deleteheaders;
        QString newHeaders = "";
        qDebug() << "*****************email col:"<<emailCol;

        for(QString receivEmail : deleteList){
            qDebug() << "*****************receive email:"<<receivEmail;
    
            if(QString::compare(emailCol,receivEmail,Qt::CaseSensitive) == 0){
                qDebug() << "mached email:"<<emailCol;
                continue;
            }
            newHeaders += receivEmail + "/";
        }
        qDebug() << "************new headers"<<newHeaders;
        string newHeadersStr = newHeaders.toStdString();
        if(newHeaders.endsWith("/")){
            newHeadersStr = newHeadersStr.substr(0,newHeadersStr.size() - 1);
        }
         bigtable.PutValue(curtUser,"email:trash",newHeadersStr);

    }else if(uri.startsWith("/email_reply")){
        QString params = uri.split("?").at(1);
        QString sender = params.split("/").at(0);
        QString subject = params.split("/").at(1);
        QFile file2(rootDir + "/html/email_reply1.html");
        if (!file2.open(QIODevice::ReadOnly | QIODevice::Text)){
            qDebug() << "file2 could not read";
            return;
        }
        page += file2.readAll();
        file2.close();
        page += "<input type=\"email\" name=\"tobox\" class=\"form-control select2-offscreen\n" 
                "id=\"tobox\" value="+sender+"@bigtable.com>\n"
                "</div>\n"
                "</div>\n"
                "    <div class=\"form-group\">\n"
                "        <label for=\"ccbox\" class=\"col-sm-1 control-label\">CC:</label>\n"
                "        <div class=\"col-sm-11\">\n"
                "              <input type=\"email\" name=\"ccbox\" class=\"form-control select2-offscreen\" id=\"ccbox\" placeholder=\"Type email\">\n"
                "        </div>\n"
                "    </div>\n"
                "    <div class=\"form-group\">\n"
                "        <label for=\"subject\" class=\"col-sm-1 control-label\">Subject:</label>\n"
                "        <div class=\"col-sm-11\">\n"
                "              <input type=\"text\" name=\"subject\" class=\"form-control select2-offscreen\" id=\"subject\" value=re:"+subject+">\n"
                "        </div></div>\n";

        QFile file3(rootDir + "/html/email_reply2.html");
        if (!file3.open(QIODevice::ReadOnly | QIODevice::Text)){
            qDebug() << "file could not read";
            return;
        }
        page += file3.readAll();
        file3.close();
    }else if(uri.startsWith("/email_forward")){
        
        QString emailCol = uri.split("?").at(1);
        qDebug() << "email column : "<<emailCol;
        
        QString subject;
        QString sender;
        QString receiver;
        QString time;

        QStringList paramList = emailCol.split("&");
        for(QString h : paramList){
            QStringList keyValue = h.split("=");
            QString key = keyValue.at(0);
            if(key.contains("sender", Qt::CaseSensitive)){
                sender = keyValue.at(1);
            }else if(QString::compare(key,"subject",Qt::CaseSensitive)==0){
                subject = keyValue.at(1);
            }else if(QString::compare(key,"time",Qt::CaseSensitive)==0){
                time = keyValue.at(1);
            }else if(QString::compare(key,"receiver",Qt::CaseSensitive)==0){
                receiver = keyValue.at(1);
            }
        }
        
        QString content = QString::fromStdString(bigtable.GetValue(curtUser,emailCol.toStdString()));
        
        QString forwardContent = "Original mail: \r\nsender:"+sender+" ";
        forwardContent += "subject:"+subject+" ";
        forwardContent += "time:"+time+" \r\n";
        forwardContent += content;

        QFile file2(rootDir + "/html/email_forward1.html");
        if (!file2.open(QIODevice::ReadOnly | QIODevice::Text)){
            qDebug() << "file2 could not read";
            return;
        }
        page += file2.readAll();
        file2.close();

        qDebug() << "************forward content:"<< forwardContent;
        
        page += "<div class=\"col-sm-11 col-sm-offset-1\">\n"
                "    <div class=\"form-group\">\n"
                "<textarea class=\"form-control\" name=\"message\" id=\"message\"\n"
                " name=\"body\" rows=\"12\">"+forwardContent+"</textarea>\n"
                "</div></div>";
                
        QFile file3(rootDir + "/html/email_forward2.html");
        if (!file3.open(QIODevice::ReadOnly | QIODevice::Text)){
            qDebug() << "file3 could not read";
            return;
        }
        page += file3.readAll();
        file3.close();        
    }

	response->setContentType("text/html");
    response->addContent(page);
    response->setStatus(200);
	
}

void EmailServlet::doPost(HttpRequest* request, HttpResponse* response) {

    HttpSession* session = request->HttpGetSession(false);
    if(session == NULL){
        QString page = "You have not log in<br>";
        page += "<a href=\"login\">log in</a>";
        
        response->setContentType("text/html");
        response->addContent(page);
        response->setStatus(200);
        return;
    }

    QString username = session->getAttribute("username");
    response->addContent(QString("Welcome! ") +username);
    string curtUser = username.toStdString();

    ServletManager* manager = ServletManager::getInstance();
    BigTableClient bigtable = manager->getBigTableClient(curtUser);

	// cout << "In emailservlet dopost!!!!!" <<endl;
	QString uri = request->getURI();
    qDebug() <<"!!!!!!!!In doGet";
    qDebug() << uri;

    QString page =  "<!DOCTYPE html><head>\n";

    if(QString::compare(uri,"/new_email",Qt::CaseSensitive) == 0){
    	QString reqStr = request->getContent();
        qDebug() << reqStr;
   		QStringList list = reqStr.split("&");
   		map<QString, QString> map; //store parameter of email: tobox, ccbox, subject,message
   		for(QString raw_param : list){
            QString param = QUrl::fromPercentEncoding(raw_param.toUtf8());
            qDebug() << "param:"<<param;
            if(param.isEmpty()){
                continue;
            }
	        QStringList keyValue = param.split("=");
	        QString key = keyValue.at(0);
	        QString value = keyValue.at(1);
	        // qDebug() << key << " is " << value;
	        if(!value.isEmpty()){
	        	map[key] = value;
	        }
        }
        
        //process the input parameters
        string senderName = curtUser; // modify to request.getSession.getParameter("user") later
        vector<QString> receivers; //receiver list 

        QString tobox = map["tobox"];

        qDebug() << "tobox:" << tobox;
        receivers.push_back(tobox);
        if(map.find("ccbox") != map.end()){
        	QStringList cclist = map["ccbox"].split(",");
        	for(QString ccbox : cclist){
        		qDebug() << "add ccbox:" << ccbox;
        		receivers.push_back(ccbox);
        	}
        }
         // generate the email headings 
		string header = "sender="+senderName;
		header += "&receiver=";
		for(int i = 0; i < receivers.size() ; i++){
			header += receivers[i].toStdString();
			if(i != receivers.size()-1){
				header += ",";
			}
		}
        header += "&subject="+map["subject"].toStdString();
		header += "&time="+getTime();
        QString messageBody = map["message"].replace("+"," ");
		string messageContent = messageBody.toStdString();


		// cout << header << endl;

        for(int i = 0; i < receivers.size() ; i++){
        	QString receiver = receivers[i];
        	string recvname = receiver.split("@").at(0).toStdString();
        	string recvhost = receiver.split("@").at(1).toStdString();
        	// cout << "receiver:" << recvname << " recvhost:"<<recvhost <<endl;
        	if(recvhost.compare("bigtable.com") == 0){
                BigTableClient bigtable_recv = manager->getBigTableClient(recvname);
        		// put value email to bigtable 
        		std::cout << "put to bigtable of user" << recvname<<std:: endl;
        		if(bigtable_recv.GetValue(recvname,"password") == "$Failed$"){
        			page += "<h3>Receiver "+receiver+" not exists in system</h3>";
        			page += "<a href = \"email\">go to mailbox</a>";
        			page += "</body></html>\n";
					response->setContentType("text/html");
					response->addContent(page);
					response->setStatus(200);
					return;
        		}

                string emailReceive = bigtable_recv.GetValue(recvname,"email:receive");
                std::cout << "email:receive:"<< emailReceive<<std::endl;
                if(emailReceive == "$Failed$"){
                    bigtable_recv.PutValue(recvname,"email:receive",header);
                    std::cout << "put email header to receiver first time "+recvname+" "+header<<std::endl;
                }else{
                    emailReceive += "/" + header;
                    bigtable_recv.PutValue(recvname,"email:receive",emailReceive);
        
                }
        		//put content to email receive 
        		bigtable_recv.PutValue(recvname,"email:receive:"+header,messageContent);
        	}else{
                string receiverStr = receiver.toStdString();
                string senderStr = senderName + "@bigtable.com";
                string subject = map["subject"].replace("+"," ").toStdString();
                cout << "******receiver:"<<receiverStr <<"sender:"<<senderStr;
                string data = "To: "+receiverStr+"\r\n";
                data += "From: "+senderStr+"\r\n";
                data += "Subject: "+subject+"\r\n\r\n";
                data += messageContent+"\r\n\r\n";

                send_msg(senderStr, receiverStr, data);
            }
        }
        // append email header to colum email:send of this user
        string eamilSendHeaders = bigtable.GetValue(senderName,"email:send");
        if(eamilSendHeaders == "$Failed$"){
            bigtable.PutValue(senderName,"email:send",header);
        }else{
            eamilSendHeaders += "/" + header;
            bigtable.PutValue(senderName,"email:send",eamilSendHeaders);
        }
        //put content to email receive 
        bigtable.PutValue(senderName,"email:send:"+header,messageContent);		

        page += "Your email have been sent!<br>";
        page += "<a href = \"email\">go to mailbox</a>";
    }

    page += "<a href = \"logout\">log out</a>";
    page += "</body></html>\n";
    response->setContentType("text/html");
    response->addContent(page);
    response->setStatus(200);

}

string getTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}

