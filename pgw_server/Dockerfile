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

# Копируем CMakeLists.txt
COPY CMakeLists.txt /app/

# Копируем директорию pgw_server
COPY pgw_server/ /app/pgw_server/

# Копируем другие необходимые директории, которые могут быть использованы в CMakeLists.txt
COPY pgw_client/ /app/pgw_client/
COPY pgw_flood_client/ /app/pgw_flood_client/

# Собираем приложение
RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    make pgw_server

# Используем тот же образ GCC для рантайма для совместимости библиотек
FROM gcc:latest

WORKDIR /app

# Установить минимальные рантайм-зависимости
RUN apt-get update && apt-get install -y \
    libboost-system-dev \
    libboost-thread-dev \
    libboost-filesystem-dev \
    libboost-program-options-dev \
    libcurl4-openssl-dev \
    libjsoncpp-dev \
    libbrotli-dev \
    --no-install-recommends \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Создать необходимую структуру директорий
RUN mkdir -p /app/config

# Копируем собранный исполняемый файл и конфиг
COPY --from=build /app/build/pgw_server /app/
COPY pgw_server/config/server_config.json /app/config/

# Открываем порт для метрик
EXPOSE 9101

# Команда для запуска сервера
CMD ["/app/pgw_server"] 