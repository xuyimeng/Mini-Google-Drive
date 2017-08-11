#include "storageservlet.h"
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


StorageServlet::StorageServlet(QObject *parent) : Servlet(parent){

}

void StorageServlet::doGet(HttpRequest* request, HttpResponse* response) {
	qDebug() << "In StorageServlet doGet";
	BigTableClient bigtable(
	grpc::CreateChannel("localhost:8001",
	                      grpc::InsecureChannelCredentials()));

    QFile file(rootDir+"/html/storage_common.html");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
            qDebug() << "file could not read";
            return;
        }
    QByteArray page = file.readAll();
        file.close();

        QString uri = request->getURI();
        qDebug() << uri;

    if(QString::compare(uri,"/upload_file",Qt::CaseSensitive) == 0) {
    	QFile file(rootDir+"/html/upload_file.html");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
            qDebug() << "file could not read";
            return;
        }
               
        page += file.readAll();
        file.close();
    } else {
        page += "</div></div></body></html>\n";
    }

    // // add more
    // if(QString::compare(uri,"/upload_file",Qt::CaseSensitive) == 0){

    //     QFile file2(rootDir+"/html/upload_file.html");
    //     if (!file2.open(QIODevice::ReadOnly | QIODevice::Text)){
    //         qDebug() << "file2 could not read";
    //         return;
    //     }
    //     page += file2.readAll();
    //     file2.close();
    // }

    qDebug() << page;
    response->setContentType("text/html");
    response->addContent(page);
    response->setStatus(200);
}

void StorageServlet::doPost(HttpRequest* request, HttpResponse* response) {
	qDebug() << "In StorageServlet doPost";
	BigTableClient bigtable(
	grpc::CreateChannel("localhost:8001",
	                      grpc::InsecureChannelCredentials()));
	QString uri = request->getURI();
    qDebug() << uri;

    QString page = "<!DOCTYPE html><head>\n";

    if(QString::compare(uri,"/upload_file",Qt::CaseSensitive) == 0) {
    	qDebug() << "store uploaded file....";
        // response->setStatus(200);
        // QTcpSocket* soc = request->getRequestSocket();
        // response->commit();
        // response->reset();
        qDebug() << request->getContent();

     //    QByteArray content = QByteArray();
     //    int requested_content_length = request->getHeaderValue("content-length").toLong();
     //    while (content.size() < requested_content_length) {
     //        if (!soc->bytesAvailable())
     //            soc->waitForReadyRead(30000);
     //        content.append(soc->readAll());
     //    }
    	// qDebug() << content;
        // Successful response page
        page += "File has been successfully uploaded!<br>";
        // page += "<a href = \"email\">go to mailbox</a>";
        // page = "<!DOCTYPE html><body>File has been successfully uploaded!<br>";
        page += "<a href= \"storage\">Go to Storage home</a></body></html>";
        qDebug() << page;
  //   	// QList<QByteArray> keyValue = reqStr.split("=");
  //   	// if(keyValue.)
  //   	qDebug() << "here1";
  //   	QString contentlen = request->getHeaderValue("content-length");
  //   	qDebug() << "here2";
  //   	qDebug() << contentlen; 
  //   	qDebug() << "here3";
  //   	QByteArray filecontent = request->getContent();
  // //   	QFile fileupload(rootDir+"/files/download.txt");
		// // if(!fileupload.open(QIODevice::WriteOnly)){
		// // 	qDebug() << "fileupload place could not read";
		// // }
		// // qDebug() << "writing file to download.txt";
		// // fileupload.write(filecontent);
		// // fileupload.close();
  //   	qDebug() << filecontent;
  //   	page = "<!DOCTYPE html><body>File has been successfully uploaded!<br>";
  //   	page = "<a href= \"storage\">Go to Storage home</a></body></html>";
    }
    page += "</body></html>\n";
    response->setContentType("text/html");
    response->addContent(page);
    qDebug() << page;
    response->setStatus(200);
    // response->commit();
}











