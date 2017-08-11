#include "loginservlet.h"
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



using namespace std;
using std::string;

LoginServlet::LoginServlet(QObject *parent) : Servlet(parent)
{

}

void LoginServlet::doGet(HttpRequest* request, HttpResponse* response) {
  
    
    QString uri = request->getURI();
    qDebug() <<"!!!!!!!!In doGet";
    qDebug() << uri;
    
   if(QString::compare(uri,"/login",Qt::CaseSensitive) == 0){
        HttpSession* session = request->HttpGetSession(false);
        QString page = "";
        if(session == NULL){
             page = "<!DOCTYPE html><body>\n"
                   "  <section class=\"container\">\n"
                   "    <div class=\"login\">\n"
                   "      <h1>Login to Web App</h1>\n"
                   "      <form method=\"post\" action=\"login\">\n"
                   "        <p><input type=\"text\" name=\"username\" value=\"\" placeholder=\"Username or Email\"></p>\n"
                   "        <p><input type=\"password\" name=\"password\" value=\"\" placeholder=\"Password\"></p>\n"
                   "        <p class=\"remember_me\">\n"
                   "          <label>\n"
                   "            <input type=\"checkbox\" name=\"remember_me\" id=\"remember_me\">\n"
                   "            Remember me on this computer\n"
                   "          </label>\n"
                   "        </p>\n"
                   "        <p class=\"submit\"><input type=\"submit\" value=\"Login\"></p>\n"
                   "      </form>\n"
                  "    </div></html>";
                  response->setContentType("text/html");
                  response->addContent(page);
                  response->setStatus(200);
                  return;
          
        }else{
              QString username = session->getAttribute("username");
              if(username.compare("")==0){
                  page = "<!DOCTYPE html><body>\n"
                   "  <section class=\"container\">\n"
                   "    <div class=\"login\">\n"
                   "      <h1>Login to Web App</h1>\n"
                   "      <form method=\"post\" action=\"login\">\n"
                   "        <p><input type=\"text\" name=\"username\" value=\"\" placeholder=\"Username or Email\"></p>\n"
                   "        <p><input type=\"password\" name=\"password\" value=\"\" placeholder=\"Password\"></p>\n"
                   "        <p class=\"remember_me\">\n"
                   "          <label>\n"
                   "            <input type=\"checkbox\" name=\"remember_me\" id=\"remember_me\">\n"
                   "            Remember me on this computer\n"
                   "          </label>\n"
                   "        </p>\n"
                   "        <p class=\"submit\"><input type=\"submit\" value=\"Login\"></p>\n"
                   "      </form>\n"
                  "    </div></html>";
                  response->setContentType("text/html");
                  response->addContent(page);
                  response->setStatus(200);
                  return;
              }
              page += QString("You have already logged in ") +username;
              page += "<br><a href=\"logout\">Log out</a>";
              response->setContentType("text/html");
              response->addContent(page);
              response->setStatus(200);
              return;
        }
        

   }else if(QString::compare(uri,"/signup",Qt::CaseSensitive) == 0){
       QString page = "<!DOCTYPE html><body>\n"
       "  <section class=\"container\">\n"
       "    <div class=\"login\">\n"
       "      <h1>Login to Web App</h1>\n"
       "      <form method=\"post\" action=\"signup\">\n"
       "        <p><input type=\"text\" name=\"username\" value=\"\" placeholder=\"Username or Email\"></p>\n"
       "        <p><input type=\"password\" name=\"password\" value=\"\" placeholder=\"Password\"></p>\n"
       "        <p class=\"remember_me\">\n"
       "          <label>\n"
       "            <input type=\"checkbox\" name=\"remember_me\" id=\"remember_me\">\n"
       "            Remember me on this computer\n"
       "          </label>\n"
       "        </p>\n"
       "        <p class=\"submit\"><input type=\"submit\" value=\"signup\"></p>\n"
       "      </form>\n"
       "    </div></html>";
       response->addContent(page);
       response->setStatus(200);
   }else if(QString::compare(uri,"/home",Qt::CaseSensitive) == 0){
       // QString page = "<!DOCTYPE html><body>\n";
       // HttpSession session = request->getSession(false); 
       // if(session.isValid()){
       //    qDebug() << "session is valid";
       //    username = session.getAttribute("username");
       //    qDebug() <<"get username from session:" +username;
       // }else{
       //    qDebug() << "session is invalid";
       // }

            
       // page += "Welcome! "+username + "<br>";
       // page += "<a href=\"email\">email</a><br>";
       // page += "<a href=\"storage\">storage</a>";
       // response->addContent(page);
       // response->setStatus(200);
        QByteArray page;
        HttpSession* session = request->HttpGetSession(false);
        if (session != NULL) {
                QString username = session->getAttribute("username");
                response->addContent(QString("Welcome! ") +username);
                QFile file(rootDir+"/html/home.html");
                    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
                        qDebug() << "file could not read";
                        return;
                }
                   
                page = file.readAll();
        }else{
          page += "You have not log in<br>";
          page += "<a href=\"login\">log in</a>";
        }
        response->setContentType("text/html");
        response->addContent(page);
        response->setStatus(200);
   }if(QString::compare(uri,"/logout",Qt::CaseSensitive) == 0){
        HttpSession* session = request->HttpGetSession(false);
        if(session == NULL){
            QString page = "You have not logged in yet";
            response->setContentType("text/html");
            response->addContent(page);
            response->setStatus(400);
            return;
        }
        session->setInvalid();

        QString page = "You have successfully log out";
        page += "<a href=\"login\">log in again</a>";
        
        response->setContentType("text/html");
        response->addContent(page);
        response->setStatus(200);
   }
}

void LoginServlet::doPost(HttpRequest* request, HttpResponse* response) {
    QString uri = request->getURI();
    qDebug() <<"!!!!!!!!In doPost";
    qDebug() << uri;
 
    QString reqStr = request->getContent();
    QStringList list = reqStr.split("&");
    QString userName;
    QString passWord;
    for(QString param : list){
        QStringList keyValue = param.split("=");
        QString key = keyValue.at(0);
        if(QString::compare(key,"username",Qt::CaseSensitive) == 0){
            userName = keyValue.at(1);
        }else if(QString::compare(key,"password",Qt::CaseSensitive) == 0){
            passWord = keyValue.at(1);
        }
    }
    qDebug() << "*********request string :"+reqStr;
    qDebug() << "username:"+userName;
    qDebug() << "password:"+passWord;
    
    //change userName and password to c++ string
    string usernameStr = userName.toUtf8().constData();
    string passwordStr = passWord.toUtf8().constData();

    ServletManager* manager = ServletManager::getInstance();
    BigTableClient bigtable = manager->getBigTableClient(usernameStr);
    
    QString page = "<!DOCTYPE html><body>\n";
    
    if(QString::compare(uri,"/login",Qt::CaseSensitive) == 0){

        string userPassword = bigtable.GetValue(usernameStr,"password");
        if( userPassword == "$Failed$"){
            page += "<h3>User not exists in system</h3>";
            page += "<a href=\"signup\">sign up</a>";
        }else if(userPassword.compare(passwordStr) != 0){
            std::cout << "password not correct" << std::endl;
            page += "<h3>Password not correct</h3>";
            page += "<a href=\"login\">log in again</a>";
        }else{
            HttpSession* session = request->HttpGetSession(true);
            session->setAttribute("username",userName);
            page += "<p>Thank you for login!</p>";
            page += "<a href=\"home\">Go to home page</a>";
            qDebug() << "session id:"<<session->getId();
            
            
        }
        
    }else if(QString::compare(uri,"/signup",Qt::CaseSensitive) == 0){
        bigtable.PutValue(usernameStr,"password",passwordStr);
        HttpSession* session = request->HttpGetSession(true);
        session->setAttribute("username",userName);
        page += "<p>Sign up successful!</p>";
        page += "<a href=\"home\">Go to home page</a>";
    }
    page += "</body></html>";
    response->setStatus(200);
    response->addContent(page);
    response->setContentType("text/html");
}
