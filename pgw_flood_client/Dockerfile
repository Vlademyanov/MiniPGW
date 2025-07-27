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

# Копируем директорию pgw_flood_client
COPY pgw_flood_client/ /app/pgw_flood_client/

# Копируем другие необходимые директории, которые могут быть использованы в CMakeLists.txt
COPY pgw_server/ /app/pgw_server/
COPY pgw_client/ /app/pgw_client/

# Собираем приложение
RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    make pgw_flood_client

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

# Копируем собранный исполняемый файл
COPY --from=build /app/build/pgw_flood_client /app/
COPY pgw_client/config/client_config.json /app/

# Открываем порт для метрик
EXPOSE 9100

# Команда для запуска flood клиента
CMD ["/app/pgw_flood_client"] 