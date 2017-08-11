#ifndef SERVLETMANAGER_H
#define SERVLETMANAGER_H

#include <QObject>
#include <QHash>
#include <QStringList>

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

#include "servlet.h"
#include "httpsession.h"
class BigTableClient;
class ServletManager : public QObject
{
    Q_OBJECT
public:
    static ServletManager* getInstance();
    static void setUp(QObject *parent = 0);

    Servlet* getMethod(QString path);
    bool setMethod(QString path, Servlet* servlet);

    HttpSession* HttpGetSession(QString name);
    void PutSession(QString name, HttpSession* session);
    void deleteSession(QString sessionID);
    // QStringList *getIpAddress(QString username);
    // void addIpAddress(QString username, QString ipAddress);

    static ServletManager* manager;
    void updateIpMap(int key, std::vector<string> ipList);
    std::vector<string> getIpList(int key);
    string getRandomBackendIp(string username);
    bool heartbeat(string ip_addr);
    BigTableClient getBigTableClient(string username);

private:
    explicit ServletManager(QObject *parent = 0);
    ~ServletManager();

    QHash<QString, Servlet*> MethodMap;
    QHash<QString, HttpSession*> sessions;
    std::map<int, std::vector<string> > ipMap;

    // QHash<QString, QStringList*> userIpMap;
signals:

public slots:
};

#include <grpc/grpc.h>
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>
#include "bigtable.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using bigtable::Position;
using bigtable::Value;
using bigtable::Update;
using bigtable::Empty;
using bigtable::Response;
using bigtable::BigTable;

using namespace std;
using std::string;

class BigTableClient {
  public:
  BigTableClient(std::shared_ptr<Channel> channel)
    : stub_(BigTable::NewStub(channel)) {}

    void PutValue (const string& row, const string& column, const string& val) {
      ClientContext context;
      Value value;
      Empty e;
      value.mutable_pos()->set_row(row);
      value.mutable_pos()->set_col(column);
      value.set_val(val);
      Status status = stub_->PUT(&context, value, &e);
      if (!status.ok()) {
       std::cout << status.error_code() << ": " << status.error_message()
                    << std::endl;
       std::cout << "put value rpc failed." << std::endl;
      } else {
       std::cout << "put value rpc success." << std::endl;
      }
     }

     string GetValue (const string& row, const string& column) {
      ClientContext context;
      Position pos;
      Value value;
      pos.set_row(row);
      pos.set_col(column);
      Status status = stub_->GET(&context, pos, &value);
      if (!status.ok()) {
       std::cout << status.error_code() << ": " << status.error_message()
                    << std::endl;
        std::cout << "Getvalue rpc failed." << std::endl;
        return "$Failed$";
      } else {
       std::cout << "value is:" << value.val() << std::endl;
       return value.val();
      }
     }

    std::vector<string> FindIpList(const string& row) {
        std::vector<string> replica;
        // int num = (row[0] - 'a') / 7;
        ClientContext context;
        Position pos;
        pos.set_row(row);
        // pos.set_col(column);
        Response message;
        std::unique_ptr<ClientReader<Response>> reader(
            stub_->Find(&context, pos));
        // ServletManager* manager = ServletManager::getInstance();
        // std::vector<std::vector<string> >* ip_list = manager->getIpList();
        while (reader->Read(&message)) {
            replica.push_back(message.msg());
        }
        Status status = reader->Finish();
        if (!status.ok()) {
            std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
            std::cout << "add ip rpc failed." << std::endl;
        } else {
            std::cout << "add ip rpc success" << std::endl;
        }
        return replica;
    }

    void terminate() {
      ClientContext context;
      Empty e1, e2;
      Status status = stub_->Terminate(&context, e1, &e2);
      if (!status.ok()) {
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        std::cout << "rpc failed." << std::endl;
      } else {
        std::cout << "Terminate server success" << std::endl;
      }
    }

    std::vector<string> getAllIpAddress() {
      std::vector<string> ip_list;
        ClientContext context;
        Empty e;
        Response message;
        std::unique_ptr<ClientReader<Response>> reader(
            stub_->GetIpList(&context, e));
        // ServletManager* manager = ServletManager::getInstance();
        // std::vector<std::vector<string> >* ip_list = manager->getIpList();
        while (reader->Read(&message)) {
            ip_list.push_back(message.msg());
        }
        Status status = reader->Finish();
        if (!status.ok()) {
            std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
            std::cout << "get ip list rpc failed." << std::endl;
        } else {
            std::cout << "get ip list rpc success" << std::endl;
        }
        return ip_list;
    }

    // void deleteValue(const string& row, const string& column) {
    //   ClientContext context;
    //   Position pos;
    //   Empty e;
    //   pos.set_row(row);
    //   pos.set_col(column);
    //   Status status = stub_->DELETE(&context, pos, &e);
    //   if (!status.ok()) {
    //    std::cout << status.error_code() << ": " << status.error_message()
    //                 << std::endl;
    //     std::cout << "delete value rpc failed." << std::endl;
    //   } else {
    //     std::cout << "delete value rpc success." << std::endl;
    //   } 
    // }
    void deleteValue(const string& row, const string& column) {
      ClientContext context;
      Value value;
      Empty e;
      value.mutable_pos()->set_row(row);
      value.mutable_pos()->set_col(column);
      value.set_val("$deleted$");
      Status status = stub_->PUT(&context, value, &e);
      if (!status.ok()) {
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        std::cout << "delete value rpc failed." << std::endl;
      } else {
        std::cout << "delete value rpc success." << std::endl;
      }
    }

    std::vector<string> getAllData() {
          std::vector<string> file_list;
          ClientContext context;
          Empty e;
          Response message;
          std::unique_ptr<ClientReader<Response>> reader(
              stub_->GetDataList(&context, e));
          while (reader->Read(&message)) {
            std::cout << message.msg() << std::endl;
              file_list.push_back(message.msg());
          }
          Status status = reader->Finish();
          std::cout << "here" << std::endl;
          if (!status.ok()) {
              std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
              std::cout << "get ip list rpc failed." << std::endl;
          } else {
              std::cout << "get ip list rpc success" << std::endl;
          }
          return file_list;
      }

  private:
  std::unique_ptr<BigTable::Stub> stub_;
};

#endif // SERVLETMANAGER_H
