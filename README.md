# 💡 Projeto 2: Controle de Iluminação (Gateway BBB) - IoT 2026.1

Este repositório contém a **Versão 2** do sistema de controle IoT descentralizado. [cite_start]O objetivo desta atividade é evoluir a arquitetura do sistema anterior, introduzindo um Gateway Local dedicado[cite: 32]. [cite_start]Neste projeto, configuramos serviços de infraestrutura em um sistema Linux embarcado (BeagleBone Black) para centralizar a comunicação da rede de sensores e atuadores[cite: 33].

---

## 📡 1. Arquitetura do Sistema

A nova topologia de rede é estruturada da seguinte forma:
* [cite_start]**Infraestrutura Wi-Fi:** Um roteador wireless atua como Access Point, interconectando os Nodes via Wi-Fi e o Gateway via cabo Ethernet[cite: 37].
* [cite_start]**Gateway (BeagleBone Black):** Atua como o cérebro da rede local, hospedando o Broker MQTT Mosquitto e gerenciando o tráfego de mensagens[cite: 38].
* [cite_start]**Nodes (ESP32-C6):** Mantêm as funções originais de sensor (botão) e atuador (LED), mas agora apontam o endereço do Broker MQTT para o IP do novo Gateway[cite: 36].
    * **Node A (Publisher):** Lê o estado do botão físico e publica "ON" ou "OFF".
    * **Node B (Subscriber):** Assina o tópico e atua no LED recebendo mensagens do Gateway.

### 🛜 Tópico MQTT Utilizado
* `ifpb/projeto/led`

---

## 🛠️ 2. Requisitos Técnicos

* [cite_start]**Hardware:** * 02 Módulos ESP32-C6[cite: 41].
  * [cite_start]01 BeagleBone Black (BBB) com SDCard e cabo Ethernet[cite: 42, 43].
  * [cite_start]01 Roteador Wi-Fi[cite: 44].
  * [cite_start]Botão, LED e resistores do Projeto 1[cite: 45].
* [cite_start]**Software e Protocolos:** * ESP-IDF (v5.x) utilizando linguagem C[cite: 47].
  * [cite_start]Protocolo de Aplicação MQTT (`espressif/mqtt`)[cite: 49].
  * [cite_start]SO Gateway: Debian/Ubuntu for BeagleBone[cite: 50].
  * [cite_start]Broker: Mosquitto[cite: 51].

---

## 📂 3. Estrutura do Repositório

```text
nome-do-repositorio/
├── node_a_publisher/      # Código do Nó A (Botão/Interruptor)
│   ├── CMakeLists.txt
│   └── main/
│       ├── CMakeLists.txt
│       └── main.c
├── node_b_subscriber/     # Código do Nó B (LED/Atuador)
│   ├── CMakeLists.txt
│   └── main/
│       ├── CMakeLists.txt
│       └── main.c
└── README.md

#Atualizamos o sistema, instalamos o broker e as ferramentas de cliente, garantindo a inicialização automática:  

sudo apt update  # Atualiza a lista de pacotes [cite: 68, 69]
sudo apt install mosquitto mosquitto-clients -y  # Instala o broker e ferramentas [cite: 70, 71]
sudo systemctl enable mosquitto  # Garante início automático [cite: 72, 73]

#Foi criado um arquivo de configuração personalizada para liberar o acesso na porta 1883 de forma anônima:
sudo nano /etc/mosquitto/conf.d/external.conf  # Criação do arquivo [cite: 87]

#Conteúdo inserido no arquivo:
listener 1883 0.0.0.0  # Escuta em todas as interfaces de rede [cite: 90, 91]
allow_anonymous true  # Permite conexões sem usuário/senha [cite: 92, 93]

#Após salvar (Ctrl+O, Enter, Ctrl+X ), o serviço foi reiniciado:

sudo systemctl restart mosquitto  # Aplica as novas regras [cite: 96]

sudo systemctl restart mosquitto  # Aplica as novas regras [cite: 96]

| Nome | Matrícula / GitHub |
| :--- | :--- |
| José Rodolfo Rocha Filho | [@Rodolfilho](https://github.com/Rodolfilho) |
| Matheus Monteiro Maciel | [@MatheusMonteiro10](https://github.com/MatheusMonteiro10) |
| Nyedson Lorran Queiroz Barros | [@NyedsonLorran](https://github.com/NyedsonLorran) |
| Vinicius Gabriel Xavier Basilio | [@ViniciusGbasilio](https://github.com/ViniciusGbasilio) |