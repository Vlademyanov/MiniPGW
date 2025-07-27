cmake_minimum_required(VERSION 3.20)
project(MiniPGW VERSION 1.0.0 LANGUAGES CXX)

# Установка стандарта C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Опции компиляции
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0") 
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# Включение FetchContent для загрузки зависимостей
include(FetchContent)

# Поиск системных библиотек
find_package(Threads REQUIRED)

# Зависимости
# nlohmann/json
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
)

# cpp-httplib
FetchContent_Declare(
        httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
        GIT_TAG v0.14.3
)

# Google Test
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
)

# spdlog
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.12.0
)

# CURL
FetchContent_Declare(
        curl
        GIT_REPOSITORY https://github.com/curl/curl.git
        GIT_TAG curl-8_5_0
)

# Настройки для CURL
set(BUILD_CURL_EXE OFF CACHE BOOL "Don't build curl executable")
set(HTTP_ONLY ON CACHE BOOL "Only enable HTTP protocol")
set(CURL_USE_OPENSSL OFF CACHE BOOL "Don't use OpenSSL")
set(BUILD_TESTING OFF CACHE BOOL "Don't build curl tests")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries")

# Prometheus CPP
FetchContent_Declare(
        prometheus_cpp
        GIT_REPOSITORY https://github.com/jupp0r/prometheus-cpp.git
        GIT_TAG        v1.2.1
)

# Настройки Prometheus CPP - отключаем все, кроме core
set(ENABLE_TESTING OFF CACHE BOOL "Disable tests" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Disable tests" FORCE)
set(PROMETHEUS_CPP_PULL ON CACHE BOOL "Enable pull exporter" FORCE)
set(PROMETHEUS_CPP_PUSH OFF CACHE BOOL "Disable push exporter" FORCE)
set(PROMETHEUS_CPP_LOGGING OFF CACHE BOOL "Disable internal logging" FORCE)

# Загрузка всех зависимостей
FetchContent_MakeAvailable(nlohmann_json httplib googletest spdlog curl prometheus_cpp)

# PGW Server
add_executable(pgw_server
        # Основные файлы приложения
        pgw_server/main.cpp
        pgw_server/AppBootstrap.cpp
        pgw_server/AppBootstrap.h
        
        # Конфигурация
        pgw_server/config/JsonConfigAdapter.cpp
        pgw_server/config/JsonConfigAdapter.h
        
        # UDP сервер
        pgw_server/udp/UdpServer.cpp
        pgw_server/udp/UdpServer.h
        
        # HTTP сервер
        pgw_server/http/HttpServer.cpp
        pgw_server/http/HttpServer.h
        
        # Компоненты приложения
        pgw_server/application/RateLimiter.cpp
        pgw_server/application/RateLimiter.h
        pgw_server/application/SessionManager.cpp
        pgw_server/application/SessionManager.h
        pgw_server/application/GracefulShutdownManager.cpp
        pgw_server/application/GracefulShutdownManager.h
        pgw_server/application/SessionCleaner.cpp
        pgw_server/application/SessionCleaner.h
        
        # Доменные объекты
        pgw_server/domain/Session.cpp
        pgw_server/domain/Session.h
        pgw_server/domain/Blacklist.cpp
        pgw_server/domain/Blacklist.h
        pgw_server/domain/ICdrRepository.h
        pgw_server/domain/ISessionRepository.h
        
        # Репозитории
        pgw_server/persistence/InMemorySessionRepository.cpp
        pgw_server/persistence/InMemorySessionRepository.h
        pgw_server/persistence/FileCdrRepository.cpp
        pgw_server/persistence/FileCdrRepository.h
        
        # Утилиты
        pgw_server/utils/Logger.cpp
        pgw_server/utils/Logger.h
        pgw_server/utils/ServerMetrics.cpp
        pgw_server/utils/ServerMetrics.h
)

target_include_directories(pgw_server PRIVATE
        ${CMAKE_SOURCE_DIR}/pgw_server
        ${CMAKE_SOURCE_DIR}/pgw_server/domain
        ${CMAKE_SOURCE_DIR}/pgw_server/application
        ${CMAKE_SOURCE_DIR}/pgw_server/persistence
        ${CMAKE_SOURCE_DIR}/pgw_server/utils
        ${CMAKE_SOURCE_DIR}/pgw_server/config
        ${CMAKE_SOURCE_DIR}/pgw_server/udp
        ${CMAKE_SOURCE_DIR}/pgw_server/http
)

target_link_libraries(pgw_server PRIVATE
        nlohmann_json::nlohmann_json
        httplib::httplib
        spdlog::spdlog
        Threads::Threads
        prometheus-cpp::core
        prometheus-cpp::pull
)

# PGW Client
add_executable(pgw_client
        pgw_client/main.cpp
        pgw_client/PgwClient.cpp
        pgw_client/PgwClient.h
        pgw_client/ClientConfig.cpp
        pgw_client/ClientConfig.h
        pgw_client/ClientLogger.cpp
        pgw_client/ClientLogger.h
)

target_include_directories(pgw_client PRIVATE ${CMAKE_SOURCE_DIR}/pgw_client)
target_link_libraries(pgw_client PRIVATE
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
)
# PGW Flood client
add_executable(pgw_flood_client
        pgw_flood_client/main.cpp
        pgw_flood_client/FloodManager.h
        pgw_flood_client/FloodManager.cpp
        pgw_flood_client/FloodWorker.h
        pgw_flood_client/FloodWorker.cpp
        pgw_flood_client/ImsiGenerator.h
        pgw_flood_client/ImsiGenerator.cpp

        pgw_client/PgwClient.cpp
        pgw_client/PgwClient.h
        pgw_client/ClientConfig.cpp
        pgw_client/ClientLogger.cpp
        pgw_flood_client/Metrics.cpp
        pgw_flood_client/Metrics.h
)

target_include_directories(pgw_flood_client PRIVATE
        ${prometheus_cpp_SOURCE_DIR}/core/include
        ${prometheus_cpp_SOURCE_DIR}/pull/include
        ${CMAKE_SOURCE_DIR}/pgw_flood_client
        ${CMAKE_SOURCE_DIR}/pgw_client
)

target_link_libraries(pgw_flood_client PRIVATE
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
        prometheus-cpp::core
        prometheus-cpp::pull
        libcurl
)

# Unit тесты

enable_testing()

add_executable(pgw_tests
        # Основной файл тестов
        pgw_server/tests/test_main.cpp
        
        # Тесты доменных объектов
        pgw_server/tests/domain/test_Session.cpp
        pgw_server/tests/domain/test_Blacklist.cpp

        # Тесты репозиториев
        pgw_server/tests/persistence/test_InMemorySessionRepository.cpp
        pgw_server/tests/persistence/test_FileCdrRepository.cpp

        # Тесты приложения
        pgw_server/tests/application/test_SessionManager.cpp
        pgw_server/tests/application/test_GracefulShutdownManager.cpp
        pgw_server/tests/application/test_RateLimiter.cpp
        pgw_server/tests/application/test_SessionCleaner.cpp

        # Тесты утилит
        pgw_server/tests/utils/test_Logger.cpp

        # Тесты  конфигурации
        pgw_server/tests/config/test_JsonConfigAdapter.cpp

        # Тесты  HTTP
        pgw_server/tests/http/test_HttpServer.cpp

        # Тесты  UDP
        pgw_server/tests/udp/test_UdpServer.cpp

        # Конфигурация
        pgw_server/config/JsonConfigAdapter.cpp
        pgw_server/config/JsonConfigAdapter.h

        # UDP сервер
        pgw_server/udp/UdpServer.cpp
        pgw_server/udp/UdpServer.h

        # HTTP сервер
        pgw_server/http/HttpServer.cpp
        pgw_server/http/HttpServer.h

        # Компоненты приложения
        pgw_server/application/RateLimiter.cpp
        pgw_server/application/RateLimiter.h
        pgw_server/application/SessionManager.cpp
        pgw_server/application/SessionManager.h
        pgw_server/application/GracefulShutdownManager.cpp
        pgw_server/application/GracefulShutdownManager.h
        pgw_server/application/SessionCleaner.cpp
        pgw_server/application/SessionCleaner.h

        # Доменные объекты
        pgw_server/domain/Session.cpp
        pgw_server/domain/Session.h
        pgw_server/domain/Blacklist.cpp
        pgw_server/domain/Blacklist.h
        pgw_server/domain/ICdrRepository.h
        pgw_server/domain/ISessionRepository.h

        # Персистентность
        pgw_server/persistence/InMemorySessionRepository.cpp
        pgw_server/persistence/InMemorySessionRepository.h
        pgw_server/persistence/FileCdrRepository.cpp
        pgw_server/persistence/FileCdrRepository.h

        # Утилиты
        pgw_server/utils/Logger.cpp
        pgw_server/utils/Logger.h
        pgw_server/utils/ServerMetrics.cpp
        pgw_server/utils/ServerMetrics.h

)

target_include_directories(pgw_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/pgw_server
        ${CMAKE_SOURCE_DIR}/pgw_server
        ${CMAKE_SOURCE_DIR}/pgw_server/domain
        ${CMAKE_SOURCE_DIR}/pgw_server/application
        ${CMAKE_SOURCE_DIR}/pgw_server/persistence
        ${CMAKE_SOURCE_DIR}/pgw_server/utils
        ${CMAKE_SOURCE_DIR}/pgw_server/config
        ${CMAKE_SOURCE_DIR}/pgw_server/udp
        ${CMAKE_SOURCE_DIR}/pgw_server/http
        ${CMAKE_SOURCE_DIR}/pgw_server/tests
)



target_link_libraries(pgw_tests PRIVATE
        gtest
        gtest_main
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        httplib::httplib
        Threads::Threads
        libcurl
)

include(GoogleTest)
gtest_discover_tests(pgw_tests)

# Копирование конфигурационных файлов в директорию сборки
configure_file(${CMAKE_SOURCE_DIR}/pgw_server/config/server_config.json
               ${CMAKE_BINARY_DIR}/server_config.json COPYONLY)
               
configure_file(${CMAKE_SOURCE_DIR}/pgw_client/config/client_config.json
               ${CMAKE_BINARY_DIR}/client_config.json COPYONLY)

# Установка
install(TARGETS pgw_server pgw_client pgw_flood_client
        RUNTIME DESTINATION bin
)

install(FILES
        pgw_server/config/server_config.json
        pgw_client/config/client_config.json
        DESTINATION etc/minipgw
)