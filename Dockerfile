FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get install -y qt5-default build-essential && \
    apt-get clean && rm -rf /var/lib/apt/lists/*
WORKDIR /app/server
COPY . /app/server
RUN qmake && make
EXPOSE 1234
CMD ["./TMP"]
