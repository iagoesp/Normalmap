# Normalmap

## Instalação de dependências no Ubuntu

### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential
sudo apt install libgl1-mesa-dev
sudo apt install libglew-dev
sudo apt install libglfw3-dev
sudo apt install libglm-dev
```

## Compilação

Para compilar o projeto, execute o seguinte comando:

```bash
g++ -o computenormals computenormals.cpp -lGL -lGLEW -lglfw -std=c++11
```

## Execução

Após compilar, execute o programa:

```bash
./computenormals
```

## Controles de Câmera

- **WASD**: Move a câmera
- **Mouse**: Controla a direção da câmera
- **ESC**: Fecha o aplicativo
