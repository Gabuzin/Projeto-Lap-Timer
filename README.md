
# 🏁 RaceMetrics – Sistema de Cronometragem e Telemetria

Sistema portátil de baixo custo para cronometragem de voltas e análise de desempenho em circuitos fechados, desenvolvido para pilotos amadores de kart, motocicletas e automóveis.

## 📌 Sobre o projeto

O **RaceMetrics** é um sistema que registra os tempos de volta e coleta informações de localização, velocidade e movimento do veículo durante uma sessão.

Diferentemente dos cronômetros convencionais, que apresentam apenas o tempo de cada volta, o RaceMetrics busca fornecer informações adicionais para auxiliar na análise do desempenho do piloto, permitindo comparar voltas e identificar diferenças ao longo da sessão.

O projeto integra sensores GNSS e IMU, microcontroladores e uma aplicação web desenvolvida em Python e Streamlit.

## 🎯 Objetivos

- Obter informações de localização e velocidade por meio do GNSS.
- Registrar e calcular os tempos de volta.
- Identificar a melhor volta da sessão.
- Coletar informações de movimento por meio da IMU.
- Armazenar os dados de cada sessão.
- Exportar os arquivos para análise externa.
- Comparar o desempenho entre diferentes voltas.

## 🏗️ Arquitetura do sistema

O RaceMetrics utiliza uma arquitetura distribuída, dividida em quatro camadas:

| Camada | Componente | Função |
|---|---|---|
| Aquisição | GNSS GN04D | Fornece localização e velocidade |
| Aquisição | IMU ICM-20948 | Coleta informações de movimento e orientação |
| Processamento | STM32F103 | Recebe e processa os dados, registra tempos e calcula métricas |
| Interface | ESP32 CYD | Apresenta informações na tela e permite interação com o usuário |
| Análise externa | Python + Streamlit | Processa os arquivos exportados e apresenta os resultados |

### 🔄 Fluxo de funcionamento

1. O GNSS e a IMU realizam a aquisição dos dados.
2. As informações são enviadas ao STM32F103.
3. O STM32 processa os dados e realiza o controle da cronometragem.
4. O ESP32 CYD recebe as informações e apresenta as métricas ao usuário.
5. Os dados da sessão podem ser exportados pelo equipamento.
6. Os arquivos exportados são carregados no dashboard para análise.

## 🔧 Hardware utilizado

| Componente | Descrição |
|---|---|
| STM32F103 | Microcontrolador responsável pelo processamento |
| ESP32 CYD | Interface com display integrado |
| GNSS GN04D | Receptor de posicionamento por satélite |
| ICM-20948 | Sensor inercial de 9 eixos |
| Alimentação 5 V | Alimentação do sistema |

## 💻 Tecnologias utilizadas

- **C/C++:** desenvolvimento do firmware dos microcontroladores.
- **UART:** comunicação serial entre os módulos.
- **I²C:** comunicação entre o STM32 e a IMU.
- **DMA:** mecanismo utilizado na recepção de dados GNSS pelo STM32.
- **Python:** processamento e análise dos dados exportados.
- **Streamlit:** desenvolvimento da interface web.
- **Pandas e NumPy:** manipulação e processamento dos dados.
- **Plotly:** geração de gráficos interativos.
- **Folium:** visualização geográfica dos trajetos.
- **Streamlit-Folium:** integração dos mapas com o dashboard.

## 📊 Dashboard de análise

O projeto possui uma aplicação web desenvolvida com Python e Streamlit, utilizada para visualizar e analisar os dados registrados durante as sessões.

Entre as funcionalidades do dashboard estão:

- Carregamento dos arquivos de telemetria.
- Visualização do histórico de voltas.
- Identificação da melhor volta.
- Comparação dos tempos de volta.
- Gráficos de velocidade e delta.
- Visualização do percurso em mapa.
- Gráficos dos dados da IMU, incluindo Roll, Pitch e Yaw.
- Visualização de altitude, HDOP e quantidade de satélites.

A análise é realizada após a sessão, sem exigir conexão com a internet durante o funcionamento do equipamento.

## 🚀 Como executar o dashboard localmente

### Pré-requisitos

- Python instalado.
- Git instalado.
- Repositório do RaceMetrics.

### 1. Clonar o repositório

```bash
git clone https://github.com/Gabuzin/Projeto-Lap-Timer.git
```

### 2. Acessar a pasta do dashboard

```bash
cd Projeto-Lap-Timer/dashboard
```

### 3. Instalar as dependências

```bash
pip install -r requirements.txt
```

### 4. Executar a aplicação

```bash
python -m streamlit run app.py
```

Após a inicialização, acesse o endereço disponibilizado pelo Streamlit no navegador:

```text
http://localhost:8501
```

## ☁️ Publicação na internet

O dashboard também pode ser executado por meio do **Streamlit Community Cloud**, conectado ao repositório do GitHub.

Para a publicação, é necessário selecionar:

- O repositório do projeto.
- A branch desejada.
- O arquivo principal da aplicação: `dashboard/app.py`.

As dependências devem estar declaradas no arquivo `dashboard/requirements.txt`.

## ⚠️ Estado atual e limitações

O projeto possui a estrutura principal de aquisição, processamento, interface e análise externa em desenvolvimento.

Algumas etapas ainda necessitam de validação integrada e em pista. Entre as limitações atuais estão:

- A precisão depende da cobertura e da qualidade do sinal GNSS.
- Os dados de orientação dependem da montagem e calibração da IMU.
- A autonomia e a resistência à vibração e à temperatura ainda precisam ser avaliadas.
- A precisão dos tempos e das informações coletadas precisa ser comparada com equipamentos de referência.

O projeto não deve ser considerado um sistema de cronometragem homologado para competições.

## 🔮 Próximos passos

- Desenvolver uma PCB dedicada para organizar as conexões elétricas.
- Melhorar a confiabilidade do hardware.
- Comparar os tempos e dados coletados com um equipamento de referência validado.

## 👨‍💻 Autores

- André Jorge Othon Teixeira
- Gabriel Victorio Ditomaso

## 🔗 Repositório

[GitHub – RaceMetrics / Projeto Lap Timer](https://github.com/Gabuzin/Projeto-Lap-Timer)

---

**RaceMetrics** – Aquisição de dados, cronometragem e análise de desempenho em circuitos fechados.
