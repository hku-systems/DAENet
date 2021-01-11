
all: p2p push

CUR=$(pwd)

include config.sh

p2p:
	# build image
	# cd ../
	docker build -t ${IMAGE}:${VERSION} ../
	# cd $CUR

registry:
	# deploy registry
	docker service create --name registry --publish published=5000,target=5000 registry:2

push:
	# tag with prefix
	docker image tag ${IMAGE}:${VERSION} ${REGISTRY}/${IMAGE}:${VERSION}

	# push image
	docker image push ${REGISTRY}/${IMAGE}:${VERSION}