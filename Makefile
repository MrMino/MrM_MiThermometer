build:
	docker run -ti --user $$(id -u):$$(id -g) \
	-v $$(pwd)/src:/code \
	-w /code \
	-e TEL_PATH=/opt/Telink_825X_SDK \
	skaldo/telink-sdk:0.1 \
	make
