docker rm `docker ps --no-trunc -aq`
docker rmi $(docker images -q)
