if [ "$1" != "" ]; then
  port="$1"
else
  port="default_port"
fi

server_ip="127.0.0.1"

server_path="build/server"
client_path="build/client"

$server_path $port &

sleep 1

$client_path $server_ip $port