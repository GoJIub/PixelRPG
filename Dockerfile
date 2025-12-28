# Базовый образ с GCC (Debian-based)
FROM gcc:latest

# Устанавливаем только то, что действительно нужно
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        cmake \
        make \
        libsfml-dev \
        fonts-liberation \
        libgl1-mesa-dri && \
    rm -rf /var/lib/apt/lists/*

# Рабочая директория
WORKDIR /workspaces/PixelRPG

# Копируем исходники
COPY . .

# Создаём и переходим в build-директорию
RUN mkdir -p build
WORKDIR /workspaces/PixelRPG/build

# Сборка проекта при создании образа
RUN cmake .. && make

# Команда по умолчанию — запуск бинарника
CMD ["./PixelRPG"]