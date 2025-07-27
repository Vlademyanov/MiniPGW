# Mini-PGW

Упрощённая модель сетевого компонента PGW (Packet Gateway) для управления сессиями мобильных абонентов.

## Что делает проект

**pgw_server** — основной сервер, который:
- Принимает UDP-запросы с IMSI абонентов
- Создаёт/отклоняет сессии на основе чёрного списка
- Ведёт CDR-журнал всех операций
- Предоставляет HTTP API для мониторинга
- Автоматически удаляет истёкшие сессии

**pgw_client** — тестовый клиент для отправки запросов на сервер

## Быстрый запуск

### 1. Сборка проекта
```bash
mkdir build && cd build
cmake ..
make
```

### 2. Настройка сервера
Создайте `config/server_config.json`:
```json
{
  "udp_ip": "0.0.0.0",
  "udp_port": 9000,
  "http_port": 8080,
  "session_timeout_sec": 30,
  "cdr_file": "cdr.log",
  "log_file": "pgw.log",
  "log_level": "INFO",
  "graceful_shutdown_rate": 10,
  "blacklist": [
    "001010123456789",
    "001010000000001"
  ]
}
```

### 3. Запуск сервера
```bash
./pgw_server
```

### 4. Тестирование клиентом
Создайте `config/client_config.json`:
```json
{
  "server_ip": "127.0.0.1",
  "server_port": 9000,
  "log_file": "client.log",
  "log_level": "INFO"
}
```

Запустите клиент:
```bash
./pgw_client 001010123456780
```

## Использование

### Тестирование с клиентом
```bash
# Отправка запроса с валидным IMSI
./pgw_client 001010123456780
# Ответ: created

# Отправка запроса с IMSI из чёрного списка
./pgw_client 001010123456789
# Ответ: rejected

# Повторный запрос с тем же IMSI
./pgw_client 001010123456780
# Ответ: created (сессия уже существует)
```

### HTTP API

**Проверка статуса абонента:**
```bash
curl "http://localhost:8080/check_subscriber?imsi=001010123456780"
# Ответ: active или not active
```

**Остановка сервера:**
```bash
curl http://localhost:8080/stop
# Ответ: Graceful shutdown initiated
```

**Проверка работоспособности:**
```bash
curl http://localhost:8080/health
# Ответ: OK
```

## Конфигурация

### Параметры сервера (server_config.json)

| Параметр | Описание | По умолчанию |
|----------|----------|--------------|
| `udp_ip` | IP-адрес для UDP-сервера | "0.0.0.0" |
| `udp_port` | Порт UDP-сервера | 9000 |
| `http_port` | Порт HTTP API | 8080 |
| `session_timeout_sec` | Таймаут сессии в секундах | 30 |
| `cleanup_interval_sec` | Интервал проверки истёкших сессий | 5 |
| `max_requests_per_minute` | Лимит запросов в минуту на IMSI | 100 |
| `cdr_file` | Путь к файлу CDR | "cdr.log" |
| `log_file` | Путь к файлу логов | "pgw.log" |
| `log_level` | Уровень логирования | "INFO" |
| `graceful_shutdown_rate` | Скорость отключения сессий/сек | 10 |
| `shutdown_timeout_sec` | Таймаут graceful shutdown | 30 |
| `blacklist` | Массив заблокированных IMSI | [] |

### Параметры клиента (client_config.json)

| Параметр | Описание | По умолчанию |
|----------|----------|--------------|
| `server_ip` | IP-адрес сервера | "127.0.0.1" |
| `server_port` | Порт сервера | 9000 |
| `log_file` | Путь к файлу логов | "client.log" |
| `log_level` | Уровень логирования | "INFO" |

## Формат данных

### UDP-протокол
- **Запрос**: IMSI в BCD-кодировке согласно TS 29.274 §8.3
- **Ответ**: ASCII строка `created` или `rejected`

### CDR-записи
Формат: `timestamp,IMSI,action`

Примеры записей в `cdr.log`:
```
2025-01-15 10:30:15,001010123456780,create
2025-01-15 10:30:20,001010123456789,rejected_blacklist
2025-01-15 10:30:45,001010123456780,timeout
2025-01-15 10:31:00,001010987654321,rejected_rate_limit
```

Возможные действия:
- `create` — создание сессии
- `rejected_blacklist` — отклонено по чёрному списку
- `rejected_rate_limit` — превышен лимит запросов
- `timeout` — сессия удалена по таймауту
- `graceful_shutdown` — сессия удалена при остановке сервера

## Логи

### Уровни логирования
- `DEBUG` — детальная отладочная информация
- `INFO` — общая информация о работе
- `WARN` — предупреждения
- `ERROR` — ошибки
- `CRITICAL` — критические ошибки

### Примеры записей
```
2025-01-15 10:30:15 [INFO] UDP server started on 0.0.0.0:9000
2025-01-15 10:30:16 [INFO] HTTP server started on 0.0.0.0:8080
2025-01-15 10:30:20 [INFO] New session created for IMSI: 001010123456780
2025-01-15 10:30:25 [WARN] Rate limit exceeded for IMSI: 001010987654321
2025-01-15 10:30:30 [INFO] Session removed: 001010123456780 (timeout)
```

## Архитектура

### UML диаграмма классов

```mermaid
classDiagram
    class AppBootstrap {
        -running: atomic<bool>
        -config: JsonConfigAdapter
        -logger: Logger
        -sessionManager: SessionManager
        -udpServer: UdpServer
        -httpServer: HttpServer
        -sessionCleaner: SessionCleaner
        -shutdownManager: GracefulShutdownManager
        +initialize()
        +run()
        +initiateShutdown()
    }

    class UdpServer {
        -ip: string
        -port: uint16_t
        -socket: int
        -running: atomic<bool>
        +start(): bool
        +stop()
        -handleIncomingPacket()
        -extractImsiFromBcd(): string
    }

    class HttpServer {
        -ip: string
        -port: uint16_t
        -running: atomic<bool>
        +start(): bool
        +stop()
        -handleCheckSubscriber()
        -handleStopCommand()
    }

    class SessionManager {
        +createSession(imsi): SessionResult
        +isSessionActive(imsi): bool
        +removeSession(imsi): bool
        +cleanExpiredSessions(): size_t
        +getAllActiveImsis(): vector<string>
    }

    class Session {
        -imsi: string
        -createdAt: time_point
        +getImsi(): string
        +isExpired(timeout): bool
        +getAge(): seconds
    }

    class ISessionRepository {
        <<interface>>
        +addSession(session): bool
        +removeSession(imsi): bool
        +sessionExists(imsi): bool
        +getExpiredSessions(): vector<Session>
    }

    class InMemorySessionRepository {
        -sessions: unordered_map<string, Session>
        -mutex: mutex
        +addSession(session): bool
        +removeSession(imsi): bool
        +sessionExists(imsi): bool
    }

    class ICdrRepository {
        <<interface>>
        +writeCdr(imsi, action): bool
        +writeCdr(imsi, action, timestamp): bool
    }

    class FileCdrRepository {
        -filePath: string
        -file: ofstream
        -mutex: mutex
        +writeCdr(imsi, action): bool
    }

    class Blacklist {
        -blacklistedImsis: unordered_set<string>
        +isBlacklisted(imsi): bool
        +setBlacklist(imsis)
    }

    class RateLimiter {
        -buckets: unordered_map<string, TokenBucket>
        -tokenRate: double
        -maxTokens: double
        +allowRequest(imsi): bool
    }

    class TokenBucket {
        +tokens: double
        +tokenRate: double
        +maxTokens: double
        +lastRefillTime: time_point
    }

    class SessionCleaner {
        -sessionTimeout: seconds
        -cleanupInterval: seconds
        -running: atomic<bool>
        +start(): bool
        +stop()
        -cleanerWorker()
    }

    class GracefulShutdownManager {
        -shutdownRate: uint32_t
        -shutdownInProgress: atomic<bool>
        -shutdownComplete: atomic<bool>
        +initiateShutdown(): bool
        +waitForCompletion(): bool
    }

    class Logger {
        -logLevel: LogLevel
        -logFile: string
        +debug(message)
        +info(message)
        +warn(message)
        +error(message)
        +critical(message)
    }

    AppBootstrap --> UdpServer
    AppBootstrap --> HttpServer
    AppBootstrap --> SessionManager
    AppBootstrap --> SessionCleaner
    AppBootstrap --> GracefulShutdownManager
    AppBootstrap --> Logger

    UdpServer --> SessionManager
    HttpServer --> SessionManager
    HttpServer --> GracefulShutdownManager

    SessionManager --> ISessionRepository
    SessionManager --> ICdrRepository
    SessionManager --> Blacklist
    SessionManager --> RateLimiter

    ISessionRepository <|.. InMemorySessionRepository
    ICdrRepository <|.. FileCdrRepository

    InMemorySessionRepository --> Session
    RateLimiter --> TokenBucket

    SessionCleaner --> SessionManager
    GracefulShutdownManager --> SessionManager
```

### Основные компоненты
```
pgw_server/
├── application/
│   ├── SessionManager              # Управление сессиями
│   ├── SessionCleaner              # Очистка истёкших сессий
│   ├── GracefulShutdownManager     # Корректное завершение
│   └── RateLimiter                 # Ограничение запросов
├── domain/
│   ├── Session                     # Класс сессии
│   ├── Blacklist                   # Чёрный список
│   ├── ISessionRepository          # Интерфейс репозитория сессий
│   └── ICdrRepository              # Интерфейс CDR репозитория
├── persistence/
│   ├── InMemorySessionRepository   # Хранение сессий в памяти
│   └── FileCdrRepository           # Запись CDR в файл
├── http/
│   └── HttpServer                  # HTTP API
├── udp/
│   └── UdpServer                   # Обработка UDP-запросов
├── config/
│   └── JsonConfigAdapter           # Парсинг JSON конфигурации
├── utils/
│   └── Logger                      # Логирование
└── AppBootstrap                    # Главный класс приложения
```

### Жизненный цикл сессии

#### Сценарий 1: Создание новой сессии
```mermaid
sequenceDiagram
    participant Client as pgw_client
    participant Server as pgw_server
    participant Sessions as SessionManager
    participant CDR as CDR File

    Client->>Server: UDP: IMSI=001010123456780
    Server->>Sessions: Проверка blacklist - OK
    Server->>Sessions: Проверка rate limit - OK
    Server->>Sessions: Сессия не существует
    Sessions->>Sessions: Создание сессии
    Server->>CDR: Запись: create
    Server-->>Client: "created"
```

#### Сценарий 2: IMSI в чёрном списке

```mermaid
sequenceDiagram
    participant Client as pgw_client
    participant Server as pgw_server
    participant Sessions as SessionManager
    participant CDR as CDR File

    Client->>Server: UDP: IMSI=001010123456789
    Server->>Sessions: Проверка blacklist - BLOCKED
    Server->>CDR: Запись: rejected_blacklist
    Server-->>Client: "rejected"
```

#### Сценарий 3: Повторный запрос продлевает сессию

```mermaid
sequenceDiagram
    participant Client as pgw_client
    participant Server as pgw_server
    participant Sessions as SessionManager

    Note over Sessions: Сессия уже существует
    Client->>Server: UDP: IMSI=001010123456780
    Server->>Sessions: Сессия найдена
    Sessions->>Sessions: Обновление времени последней активности
    Server-->>Client: "created"
    Note right of Sessions: CDR запись НЕ создается
```

#### Сценарий 4: Автоматическая очистка по таймауту

```mermaid
sequenceDiagram
participant Timer as SessionCleaner
participant Sessions as SessionManager
participant CDR as CDR File

    Note over Timer: Каждые cleanup_interval_sec
    Timer->>Sessions: Поиск истёкших сессий
    Sessions-->>Timer: IMSI=001010123456780 (timeout)
    Timer->>Sessions: Удаление сессии
    Timer->>CDR: Запись: timeout
```
## Устранение проблем

### Сервер не запускается
```
ERROR: Cannot find configuration file
```
**Решение**: Создайте файл `config/server_config.json` или поместите его в корень проекта

```
ERROR: Bind failed for 0.0.0.0:9000
```
**Решение**: Порт занят, измените `udp_port` в конфигурации или остановите процесс, использующий порт

### Клиент не подключается
```
ERROR: Connection refused
```
**Решение**: Убедитесь, что сервер запущен и проверьте настройки `server_ip`/`server_port` в конфигурации клиента

### Все запросы отклоняются
**Проверьте**: 
1. Не находится ли IMSI в массиве `blacklist`
2. Не превышен ли `max_requests_per_minute`

## Тестирование

Проект включает unit-тесты для основных компонентов:

```bash
# Сборка с тестами
cmake -DBUILD_TESTS=ON ..
make

# Запуск тестов
ctest
```

## Требования

- **ОС**: Linux
- **Компилятор**: GCC/Clang с поддержкой C++20
- **Сборка**: CMake 3.10+
- **Зависимости**: автоматически загружаются через CMake

---

*Проект создан как выпускная работа C++ школы*
