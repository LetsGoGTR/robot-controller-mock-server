FROM drogonframework/drogon:latest

RUN apt-get update && apt-get install -y \
    cmake build-essential git libssl-dev libjsoncpp-dev uuid-dev zlib1g-dev libboost-all-dev libarchive-dev

WORKDIR /app
COPY . /app

RUN mkdir build && cd build && cmake .. && make

CMD ["./build/robot-controller-mock-server"]
