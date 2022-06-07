rm -f cout cerr
pkill curl
for _ in {1..10000}
do
	curl -X POST\
		-H "Content-Type: application/json"\
		-d '{ "mmid": "SyAJoqrX8r6Ycbr8bFB2", "mail": "test+'$_'@exmaple.com"  }'\
		http://localhost:8080/login/mmid \
		&& echo $_ &
done
sleep 10
echo over.
