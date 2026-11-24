# Race Metrics — arquivos no LittleFS

Esta alteração implementa a organização dos arquivos solicitada. O formato e a
frequência atuais do CSV (intervalo mínimo de 1.000 ms) continuam os mesmos.
A refatoração completa de aquisição e importação RaceChrono do documento
de referência não faz parte desta alteração.

```text
/config/
    settings.json
    pistas.json
/logs/
    session_001/
        metadata.json
        laps.json
        telemetry.csv
    session_002/
        metadata.json
        laps.json
        telemetry.csv
```

`LapData` mantém as configurações nos caminhos existentes. O equipamento liga
com a gravação parada. Em Configurações > Controle da sessão, selecione a pista
e o intervalo mínimo entre voltas e pressione Iniciar sessão. A primeira amostra
de GPS válida após esse comando cria a sessão, mesmo com velocidade zero.
Parar sessão cancela a espera pelo GPS ou encerra a gravação e mantém o
equipamento parado até um novo comando Iniciar. A perda do GPS suspende as
amostras, mas conserva a sessão aberta. Os dados ao vivo continuam atualizando
enquanto a gravação está parada. O identificador é o primeiro
`session_NNN` disponível, considerando pastas e arquivos antigos; números acima
de 999 continuam válidos. Nenhum arquivo existente é sobrescrito para criar uma
sessão. Os arquivos legados `/logs/<id>.csv` e `/logs/<id>.json` continuam
listados e disponíveis para download, sem migração destrutiva.

## Metadados

`metadata.json` contém:

| Campo | Conteúdo |
| --- | --- |
| `schemaVersion`, `telemetrySchemaVersion` | Versão 1 dos esquemas |
| `id`, `name` | Identificador e nome da sessão |
| `trackId`, `track`, `trackSnapshot` | ID, nome e cópia do cadastro da pista no início |
| `minLapSeconds` | Limite de volta usado pela sessão |
| `date`, `startedUtc` | UTC inicial; string vazia se indisponível |
| `endedUtc` | Último UTC observado ao encerrar; `null` enquanto aberta ou interrompida |
| `startedTimestampMs`, `endedTimestampMs` | Tempos locais do ESP32; término só existe após encerramento |
| `checkpointTimestampMs` | Instante local do último resumo gravado |
| `lastSampleTimestampMs` | Instante da última linha CSV confirmada naquele resumo |
| `state` | `recording`, `closed`, `interrupted` ou `error` |
| `sampleCount`, `lapCount` | Linhas CSV completas e voltas persistidas no último resumo |
| `sampleCountExact` | `true` somente após encerramento normal |
| `bestLapTimeMs` | Melhor volta detectada em ms; `null` antes da primeira |
| `loggingIntervalMs` | 1.000 ms; não representa a frequência medida do GPS |
| `telemetryFile`, `lapsFile` | `telemetry.csv`, `laps.json` |

O resumo é atualizado a cada 30 segundos de amostras válidas, ao completar uma
volta e ao encerrar a sessão. A listagem web usa contadores atuais em RAM para a
sessão ativa; o download dos metadados entrega o último resumo persistido.

Editar/excluir a pista ativa encerra a sessão antes da alteração e mantém a
gravação parada. O método `bool LapData::endSession()` também encerra a sessão
e reinicia a cronometragem. Falhas ao salvar o encerramento são retornadas ao
chamador; novas amostras deixam de ser gravadas e o usuário pode tentar Parar
novamente. Pista e intervalo ficam bloqueados no formulário durante a sessão.
Nome e senha ficam em um formulário separado e não encerram a gravação.

`POST /api/recording/start` recebe `activeTrack` e `minLapSeconds` como formulário
URL-encoded. Pista vazia grava somente telemetria. `POST /api/recording/stop`
finaliza a sessão. `GET /api/status` informa `recordingRequested` e `sessionState`
(`idle`, `waiting_gps`, `recording`, `gps_lost`, `error`, `stop_error` ou
`unavailable`). O estado de gravação não persiste entre reinicializações.
`POST /api/settings` recebe somente `deviceName` e, opcionalmente,
`devicePassword`. A pista de teste só é criada no primeiro cadastro, permitindo
que sua exclusão ou a seleção Sem pista sejam preservadas após reiniciar.

No boot, sessões que ainda estavam `recording` são marcadas `interrupted`.
Os contadores permanecem os do último resumo: podem existir linhas ou voltas
mais recentes nos arquivos. Não se inventa um horário de encerramento nem se
reescreve o CSV. Uma última linha incompleta após queda de energia deve ser
descartada pela ferramenta de análise. Não há reconstrução automática de
contadores a partir da telemetria nesta versão.

## Voltas

`laps.json` é um array inicialmente vazio. Cada volta concluída pela lógica
atual acrescenta `number`, `startTimestampMs`, `endTimestampMs`, `durationMs`,
`valid` e `gapToPreviousBestMs`. A diferença usa a melhor volta existente
antes desse cruzamento (`null` na primeira volta), sem recalcular o histórico.
Ela compara durações de voltas completas; não é um gap de posição em tempo real.
Os cruzamentos continuam respeitando direção e tempo mínimo; o instante de
passagem agora é interpolado entre as duas posições GPS. `valid` fica falso
quando há perda de fix, intervalo entre posições acima de 2,5 s ou salto de
posição incompatível com o limite de 100 m/s mais 20 m de tolerância. Essas
voltas permanecem no histórico, mas não viram a melhor volta. A volta ainda
em andamento não entra no array.

## Gap ao vivo e melhor volta no display

O display usa a melhor volta válida da sessão no formato `M:SS.mmm`. Antes da
primeira volta completa, mostra `--:--.---`. Parar conserva o resultado na tela;
Iniciar outra sessão limpa o resultado e a referência. Editar/excluir a pista
ativa também limpa essa referência.

`LapGap` guarda o traçado da volta atual e da melhor volta em dois buffers de
512 pontos (cerca de 16 KiB no total). O intervalo inicial é 200 ms, com pelo
menos 1 m percorrido entre pontos armazenados. Quando o buffer enche, mantém
um ponto a cada dois e dobra o intervalo; início e fim são preservados.
Os pontos são coordenadas locais em metros, distância acumulada e tempo desde
a linha de chegada. Só novas posições GGA alimentam o cálculo, usando o
instante local de recepção, sem repetir a posição a cada iteração da interface.

O gap projeta a posição atual nos segmentos da referência e interpola o tempo
da melhor volta naquele ponto. Resultado negativo é vantagem (verde); positivo
é atraso (vermelho); zero é neutro. A busca limita avanço e direção para reduzir
saltos para trechos próximos da pista. Fora de 25 m da referência ou sem
correspondência no trecho esperado, o display mostra `--`. Também mostra `--`
antes de haver referência, com GPS desatualizado, gravação parada ou erro.
Após perda de GPS, a volta atual não vira referência e o cálculo retoma na
próxima volta. `GET /api/status` expõe `gapSeconds` ou `null` quando indisponível.

É uma estimativa GPS, não uma medição com precisão de milissegundos garantida.
As três casas decimais são o formato de apresentação; frequência do receptor,
ruído de posição, curvas entre amostras e atrasos de recepção limitam a precisão.
O CSV e `gapToPreviousBestMs` continuam com seus formatos anteriores.

Teste local do cálculo (compilador C++ no PATH):

```text
g++ -std=c++11 -O2 -Wall -Wextra -static test/lap_gap_test.cpp -o .pio/lap_gap_test.exe
.pio/lap_gap_test.exe
```

A atualização copia o array anterior em blocos de 512 bytes para um temporário
e acrescenta a nova volta. O histórico não fica inteiro em RAM. Esse processo
exige espaço temporário equivalente ao arquivo de voltas e custa mais conforme
o histórico cresce. JSONs são substituídos por `rename` do LittleFS, sem apagar
previamente a versão confirmada. Escrita incompleta ou falha de alocação não
substitui o JSON anterior.

## Telemetria

`telemetry.csv` contém somente cabeçalho e linhas de amostras. Separador `,`,
decimal `.`, coordenadas em graus decimais, altitude em metros, velocidade em
km/h e orientação em graus. O cabeçalho preservado é:

```csv
indice_amostra,timestamp_ms,timestamp_utc_iso,segundos_desde_inicio,volta_associada,tempo_volta_s,ultima_volta_s,latitude_decimal,longitude_decimal,altitude_m,velocidade_kmh,roll_deg,pitch_deg,yaw_deg,hdop,satelites,fix_quality,track_id
```

`timestamp_ms` é o tempo local de processamento (`millis()`, 32 bits), não o
instante de aquisição sincronizado com o STM32. A primeira amostra tem índice
zero; `volta_associada=0` significa que a primeira passagem ainda não ocorreu.
Cada linha usa o `trackId` capturado na abertura da sessão. A rotina existente
abre, grava, sincroniza e fecha o CSV a cada linha. Bufferização e validade
individual de todos os canais permanecem trabalhos
separados. Não se afirma compatibilidade nativa com RaceChrono.

Em uma escrita parcial, o logger para de acrescentar dados àquela sessão, para
não concatenar a próxima linha com o fragmento. `sessionWriteFailed` aparece
em `/api/status`; a cronometragem continua. Também se interrompe a gravação
em falhas de persistência de voltas ou metadados.

## Downloads

Ative o Wi-Fi pela interface existente, conecte-se ao AP
`RaceMetrics-LapTimer`, acesse o endereço mostrado no serial e abra
**Telemetria**. Cada sessão oferece CSV, metadados e, quando disponível, voltas.
O modo AP continua desligado até ser ativado pelo usuário.

| Endpoint GET | Resultado |
| --- | --- |
| `/api/sessions` | Lista sessões nos dois layouts |
| `/api/session?id=session_001` | Telemetria CSV; compatível com o link anterior |
| `/api/session?id=session_001&file=telemetry.csv` | Telemetria CSV |
| `/api/session?id=session_001&file=metadata.json` | Metadados JSON |
| `/api/session?id=session_001&file=laps.json` | Voltas JSON; 404 para sessões antigas sem esse arquivo |

Todos os downloads usam o fluxo de arquivos do ESPAsyncWebServer. Só os três
nomes acima são aceitos (`400` para outro nome); IDs permitem apenas letras,
dígitos, `_` e `-`, com até 40 caracteres. Arquivo/ID inexistente retorna `404`.
Os métodos `getSessionPath()` e `getSessionFilePath()` mantêm essa validação.

## Validação

- Compilar firmware: `pio run -e esp32dev`.
- Testar sintaxe e links da interface: `node test/web_sessions.test.mjs`.
- No dispositivo: iniciar com configurações e CSVs legados, verificar sua
  listagem, gerar duas passagens válidas pela chegada e baixar os três arquivos.
  Conferir cabeçalho único, array de voltas e contadores após trocar a pista.
- Reiniciar durante a gravação: verificar sessão anterior `interrupted`, arquivos
  preservados e nova sessão com outro ID.
- Em uma unidade de teste com armazenamento cheio: verificar erro sinalizado,
  JSON anterior preservado e ausência de novas linhas após escrita parcial.
- Testar downloads grandes e rejeição de `id=../config`/`file=settings.json`.

Os cenários de dispositivo, falta de espaço e interrupção de energia exigem
validação física. Compilar e testar a interface não substitui esses ensaios.

Validação realizada nesta alteração: compilação `esp32dev` concluída, teste
JavaScript aprovado e `git diff --check` sem erros. RAM estática: 69.468 de
327.680 bytes (21,2%). Firmware: 1.295.613 de 1.310.720 bytes (98,8% da partição
de aplicação; não é a ocupação do LittleFS). O teste JavaScript verifica a
sintaxe completa e a renderização de sessões vazias, novas e antigas, incluindo
escape de HTML e codificação dos IDs. Não foi realizado upload no ESP32.
