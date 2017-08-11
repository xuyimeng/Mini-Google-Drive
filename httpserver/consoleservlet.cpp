#include "consoleservlet.h"
#include <QString>
#include <QByteArray>
#include <QList>
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
#include <time.h>
#include <fstream>
#include <QUrl>

string getTime();
using namespace std;


ConsoleServlet::ConsoleServlet(QObject *parent) : Servlet(parent){

}

void ConsoleServlet::doGet(HttpRequest* request, HttpResponse* response){
	qDebug() << "In ConsoleServlet doGet";
	BigTableClient masterClient(
	grpc::CreateChannel("localhost:5000",
	                      grpc::InsecureChannelCredentials()));
	std::vector<string> workers = masterClient.getAllIpAddress();
	QString page = "<!DOCTYPE html><body>\n";
                   
   	QString uri = request->getURI();
    qDebug() << uri;

    if(QString::compare(uri,"/console",Qt::CaseSensitive) == 0){
    	page += "<p>List of worker server:</p>";
    	page += "  <ul>";
		for(int i = 0; i < workers.size(); i++){
			string workerdata = workers[i];
			string workerAddr = workerdata.substr(0,workerdata.find(','));
			int status = atoi(workerdata.substr(workerdata.find(',')+1).c_str());
			page += "<li>";
			page +=  QString::fromStdString(workerAddr);
			if(status == 1){
				page += " is alive";
				QString consolequery = "terminate?"+QString::fromStdString(workerAddr);
				QString showquery = "showdata?"+QString::fromStdString(workerAddr);
				page += "<a href=\""+consolequery+"\">  terminate</a>";
				page += "<a href=\""+showquery+"\">  show data</a>";
			}else{
				page += " is down";
			}

			page += "</li>";
		} 
		page += "</ul></body></html>";

    }else if(uri.startsWith("/terminate")){
    	
    	QString workerAddr = uri.split("?").at(1);
    	BigTableClient tempClient(
		grpc::CreateChannel(workerAddr.toStdString(),
	                      grpc::InsecureChannelCredentials()));
    	tempClient.terminate();
    	page += "<a href=\"console\">go back to console</a>";

    }else if(uri.startsWith("/showdata")){
    	
    	QString workerAddr = uri.split("?").at(1);
    	BigTableClient tempClient(
		grpc::CreateChannel(workerAddr.toStdString(),
	                      grpc::InsecureChannelCredentials()));
    	page += "<a href=\"console\">go back to console</a>";
    	std::vector<string> data = tempClient.getAllData();
    	page += "<p>List of data in worker server:</p>";
    	page += "<table class=\"table table-condensed table-hover\">\n"
                "      <thead>\n"
                "        <tr>\n"
                "          <th class=\"span2\"><strong>Row Name</strong></th>\n"
                "          <th class=\"span9\"><strong>Col Name</strong></th>\n"
                "          <th class=\"span2\"><strong>Headers</strong></th>\n"
                "        </tr>\n"
                "      </thead>\n";
    	page += "  <ul>";
    	for(int i = 0; i < data.size(); i++){
    		QString row = QString::fromStdString(data[i]);
    		QString userName = row.split(",").at(0);
    		QString colName = row.split(",").at(1);
    		QString entrydata = row.split(",").at(2);

    		for(QString entry : entrydata.split("/")){
    			 if(entry.compare("")==0){
                    continue;
                }
    		QString rawdata_query="show_rawdata?workerAddr=:"+workerAddr
    							 +"/userName=:"+userName
    							 +"/colName=:"+colName+":"+entry;
    		page += " <tr>\n"
                    "      <td>"+userName+"</td>\n"
                    "      <td>"+colName+"</td>\n"
                    "      <td><a href=\""+rawdata_query+"\">"+ entry +"</a></td>\n"
                    " </tr>\n";
    		}
    	}

    }else if(uri.startsWith("/show_rawdata")){

    	QString requestParam = uri.split("?").at(1);
    	 qDebug() << "requestParam:"+requestParam;
    	  // split arguments by &
        QStringList headerList = requestParam.split("/");
        QString userName;
        QString colName;
        QString workerAddr;

        for(QString h : headerList){
        	qDebug() << "h:"<<h;
            QStringList keyValue = h.split("=:");
            QString key = keyValue.at(0);
            if(key.contains("workeraddr")){
                workerAddr = keyValue.at(1);
            }else if(key.contains("username")){
                userName = keyValue.at(1);
            }else if(key.contains("colname")){
                colName = keyValue.at(1);
            }
        }
        qDebug() << "userName:"+userName << "colName:"+colName;
    	BigTableClient tempClient(
		grpc::CreateChannel("localhost:8010",
	                      grpc::InsecureChannelCredentials()));
    	string rawdata = tempClient.GetValue(userName.toStdString(),colName.toStdString());
    	page += "<a href=\"console\">go back to console</a><br>raw data:<br>";

    	page += QString::fromStdString(rawdata);

    }
	
	response->setContentType("text/html");
    response->addContent(page);
    response->setStatus(200);

}

void ConsoleServlet::doPost(HttpRequest* request, HttpResponse* response){

}