#!/bin/bash
#系统状态信息
#DISKROUTE=`cat ylca.xml | grep DISKROUTE | awk -F'="|"' '{print $2}'`
curl http://127.0.0.1:9092/api/v1/query?query=node_memory_MemAvailable/node_memory_MemTotal 1> stateOfMem 2> /dev/null
curl -g 'http://127.0.0.1:9092/api/v1/query?query=node_filesystem_avail{mountpoint="/"}/node_filesystem_size{mountpoint="/"}' 1> stateOfDisk 2> /dev/null
curl -g 'http://127.0.0.1:9092/api/v1/query?query=sum(irate(node_cpu{instance="BSDR1system",mode="idle"}[5m]))by(instance)/scalar(count(node_cpu{mode="user",instance="BSDR1system"}))' 1> stateOfBSDR1CPU 2> /dev/null
curl -g 'http://127.0.0.1:9092/api/v1/query?query=sum(irate(node_cpu{instance="BSDR2system",mode="idle"}[5m]))by(instance)/scalar(count(node_cpu{mode="user",instance="BSDR2system"}))' 1> stateOfBSDR2CPU 2> /dev/null
curl -g 'http://127.0.0.1:9092/api/v1/query?query=sum(irate(node_cpu{instance="BSDR3system",mode="idle"}[5m]))by(instance)/scalar(count(node_cpu{mode="user",instance="BSDR3system"}))' 1> stateOfBSDR3CPU 2> /dev/null
curl -g 'http://127.0.0.1:9092/api/v1/query?query=sum(irate(node_cpu{instance="BSDR4system",mode="idle"}[5m]))by(instance)/scalar(count(node_cpu{mode="user",instance="BSDR4system"}))' 1> stateOfBSDR4CPU 2> /dev/null
curl -g 'http://127.0.0.1:9092/api/v1/query?query=sum(irate(node_cpu{instance="BSDR5system",mode="idle"}[5m]))by(instance)/scalar(count(node_cpu{mode="user",instance="BSDR5system"}))' 1> stateOfBSDR5CPU 2> /dev/null
curl -g 'http://127.0.0.1:9092/api/v1/query?query=sum(irate(node_cpu{instance="BCR1system",mode="idle"}[5m]))by(instance)/scalar(count(node_cpu{mode="user",instance="BCR1system"}))' 1> stateOfBCR1CPU 2> /dev/null
curl -g 'http://127.0.0.1:9092/api/v1/query?query=sum(irate(node_cpu{instance="BCR2system",mode="idle"}[5m]))by(instance)/scalar(count(node_cpu{mode="user",instance="BCR2system"}))' 1> stateOfBCR2CPU 2> /dev/null
curl -g 'http://127.0.0.1:9092/api/v1/query?query=sum(irate(node_cpu{instance="BCR3system",mode="idle"}[5m]))by(instance)/scalar(count(node_cpu{mode="user",instance="BCR3system"}))' 1> stateOfBCR3CPU 2> /dev/null
curl -g 'http://127.0.0.1:9092/api/v1/query?query=sum(irate(node_cpu{instance="FBAARsystem",mode="idle"}[5m]))by(instance)/scalar(count(node_cpu{mode="user",instance="FBAARsystem"}))' 1> stateOfFBAARCPU 2> /dev/null
#Ceph状态信息
ceph -s 1> stateOfCEPH 2> /dev/null
#数据库状态信息
curl http://127.0.0.1:9092/api/v1/query?query=mysql_global_status_threads_connected 1> stateOfSqlCon 2> /dev/null
#mysqladmin -uyl -p'1qaz@WSX3edc$RFVyl' -hXXX.XXX.XXX.XXX -P6033 ping > Middleware
#mysqladmin -uyl -p1qaz@WSX3edc$RFVyl -hXXX.XXX.XXX.XXX -P3306 ping > stateOfDB
#共识模块状态信息
BCR1IP=`cat ylca.xml | grep BCR1IP | awk -F'="|"' '{print $2}'`
BCR2IP=`cat ylca.xml | grep BCR2IP | awk -F'="|"' '{print $2}'`
BCR3IP=`cat ylca.xml | grep BCR3IP | awk -F'="|"' '{print $2}'`
#BCR1
curl -v  -d '{"method":"getstatus","params":[] }' -H "Authorization: Basic bm9kZWR1c3I6Y29ublRvTm9kZWQ="  http://${BCR1IP}:21000/ 1> stateOfBCR1Chain1 2> /dev/null
curl -v  -d '{"method":"getstatus","params":[] }' -H "Authorization: Basic bm9kZWR1c3I6Y29ublRvTm9kZWQ="  http://${BCR1IP}:21001/ 1> stateOfBCR1Chain2 2> /dev/null
curl -v  -d '{"method":"getstatus","params":[] }' -H "Authorization: Basic bm9kZWR1c3I6Y29ublRvTm9kZWQ="  http://${BCR1IP}:21002/ 1> stateOfBCR1Chain3 2> /dev/null
#BCR2
curl -v  -d '{"method":"getstatus","params":[] }' -H "Authorization: Basic bm9kZWR1c3I6Y29ublRvTm9kZWQ="  http://${BCR2IP}:21000/ 1> stateOfBCR2Chain1 2> /dev/null
curl -v  -d '{"method":"getstatus","params":[] }' -H "Authorization: Basic bm9kZWR1c3I6Y29ublRvTm9kZWQ="  http://${BCR2IP}:21001/ 1> stateOfBCR2Chain2 2> /dev/null
curl -v  -d '{"method":"getstatus","params":[] }' -H "Authorization: Basic bm9kZWR1c3I6Y29ublRvTm9kZWQ="  http://${BCR2IP}:21002/ 1> stateOfBCR2Chain3 2> /dev/null
#BCR3
curl -v  -d '{"method":"getstatus","params":[] }' -H "Authorization: Basic bm9kZWR1c3I6Y29ublRvTm9kZWQ="  http://${BCR3IP}:21000/ 1> stateOfBCR3Chain1 2> /dev/null
curl -v  -d '{"method":"getstatus","params":[] }' -H "Authorization: Basic bm9kZWR1c3I6Y29ublRvTm9kZWQ="  http://${BCR3IP}:21001/ 1> stateOfBCR3Chain2 2> /dev/null
curl -v  -d '{"method":"getstatus","params":[] }' -H "Authorization: Basic bm9kZWR1c3I6Y29ublRvTm9kZWQ="  http://${BCR3IP}:21002/ 1> stateOfBCR3Chain3 2> /dev/null









