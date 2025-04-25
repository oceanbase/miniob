#!/bin/bash
set -x
function init_coredump_settings()
{
  sudo mkdir -p /corefile
  sudo chmod 777 /corefile
  sudo ulimit -c unlimited

  # 这两个配置在容器中不生效，需要在宿主机上配置
  sudo sysctl -w kernel.core_uses_pid=1
  sudo sysctl -w kernel.core_pattern=/corefile/core.%p
}

function is_mysql_server_up()
{
  mysqladmin ping
  return $?
}

function start_mysql_server()
{
  is_mysql_server_up
  ret=$?
  if [ $ret -eq 0 ];
  then
    echo "mysql server started"
  else
    # 初始化多次会报错，但是不会覆盖
    sudo mysqld --initialize-insecure
    sudo nohup mysqld --user=root &
    # sleep 一段时间。因为使用脚本执行命令时，上一行命令可能还没有就绪
    sleep 10
    is_mysql_server_up
    ret=$?
    echo "mysql server start done. ret =" $ret

  fi
  return $ret
}

function main()
{
  init_coredump_settings
  start_mysql_server
}

main
