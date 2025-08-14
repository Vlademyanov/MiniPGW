FROM gcc:latest AS build

WORKDIR /app

# Установить зависимости
RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    libboost-all-dev \
    libcurl4-openssl-dev \
    libjsoncpp-dev \
    git \
    --no-install-recommends \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Копируем  CMakeLists.txt
COPY CMakeLists.txt /app/

# Копируем директорию pgw_client
COPY pgw_client/ /app/pgw_client/

# Копируем другие необходимые директории, которые могут быть использованы в CMakeLists.txt
COPY pgw_server/ /app/pgw_server/
COPY pgw_flood_client/ /app/pgw_flood_client/

# Собираем приложение
RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    make pgw_client

# Используем тот же образ для рантайма для совместимости библиотек
FROM gcc:latest

WORKDIR /app

# Установить рантайм-зависимости
RUN apt-get update && apt-get install -y \
    libboost-system-dev \
    libboost-thread-dev \
    libboost-filesystem-dev \
    libboost-program-options-dev \
    libcurl4-openssl-dev \
    libjsoncpp-dev \
    --no-install-recommends \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Копируем собранный исполняемый файл и конфиг
COPY --from=build /app/build/pgw_client /app/
COPY pgw_client/config/client_config.json /app/

# Команда для запуска клиента
CMD ["/app/pgw_client"]