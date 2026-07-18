# Monitor Rejoin BI para ESP32

Versão estável atual: **v1.0**.

Firmware independente para verificar periodicamente se uma conta consegue manter uma sessão válida na plataforma Rejoin BI.

O projeto não contém URL de cliente, e-mail, senha de plataforma ou senha de Wi-Fi. Cada dispositivo é configurado pelo próprio cliente em uma página local.

## Como o monitor funciona

1. O ESP32 cria uma rede Wi-Fi temporária de configuração.
2. O cliente informa o Wi-Fi, o endereço HTTPS da plataforma e os pinos dos LEDs.
3. O cliente entra com e-mail e senha na página local.
4. Se a plataforma solicitar PIN, a mesma página apresenta o campo para o código enviado por e-mail.
5. Após o login, a senha é apagada da memória e somente o cookie de sessão é salvo.
6. O ESP32 consulta `/plataforma/api/check-session` no intervalo configurado.
7. Sessão válida mantém o LED saudável aceso. Falha de rede, sessão ou plataforma faz o LED de erro piscar rapidamente.

## Primeiro acesso

Abra o Monitor Serial em `115200 baud` após gravar. O dispositivo mostrará algo semelhante a:

```text
Rede Wi-Fi : RejoinBI-IOT-8A3A14
Senha      : RJ348A3A14
Endereço   : http://192.168.4.1
```

Conecte o celular ou computador nessa rede e abra `http://192.168.4.1`. Alguns aparelhos abrem o portal automaticamente.

Na página:

1. Clique em **Pesquisar redes** e selecione uma rede Wi-Fi de 2,4 GHz encontrada pelo ESP32.
2. Informe a senha dessa rede. O nome ainda pode ser digitado manualmente para redes ocultas.
3. Informe apenas o endereço raiz da plataforma, por exemplo `https://empresa.rejoinbi.com.br`.
4. Escolha o intervalo de verificação.
5. Configure o tipo e os GPIOs dos LEDs.
6. Clique em **Salvar e conectar** e aguarde o Wi-Fi ficar conectado.
7. Entre com a conta da plataforma e, se solicitado, confirme o PIN.

O ESP32 clássico não suporta redes exclusivamente 5 GHz. Se a rede não aparecer na pesquisa, habilite a faixa 2,4 GHz do roteador, aproxime a placa ou verifique se o ponto de acesso está oculto.

O ponto de configuração é desligado alguns segundos após o login. Para abri-lo novamente, pressione o botão `BOOT` por três segundos com o ESP32 já ligado.

## LEDs compatíveis

### Um LED comum

É o modo inicial, compatível com a maioria dos ESP32 clássicos:

- Aceso continuamente: plataforma e sessão funcionando.
- Piscando rapidamente: falha de Wi-Fi, plataforma ou sessão.
- Piscando lentamente: configurando ou conectando.

O ESP32 clássico normalmente tem somente um LED programável no GPIO 2. O LED vermelho da placa costuma ser ligado diretamente à alimentação e não pode mudar de estado pelo firmware.

### Dois LEDs

Para obter exatamente verde e vermelho, use dois LEDs programáveis da placa ou dois LEDs externos. Uma ligação segura comum é:

- LED vermelho: GPIO 25, resistor de 220 a 330 ohms, LED e GND.
- LED verde: GPIO 26, resistor de 220 a 330 ohms, LED e GND.
- Na página, selecione **Dois LEDs, vermelho e verde** e mantenha a lógica invertida desativada.

Confirme o pinout da sua placa antes de ligar qualquer componente. Não ligue um LED diretamente ao GPIO sem resistor.

### LED RGB endereçável

Placas com WS2812 integrado podem usar o modo **LED RGB endereçável**. Informe o GPIO indicado no pinout da placa. O firmware usa a biblioteca Adafruit NeoPixel.

## Segurança

- A senha da plataforma nunca é gravada na memória permanente.
- E-mail e cookie de sessão são guardados no NVS do ESP32 para manter o monitor após reinicializações.
- A senha do Wi-Fi precisa ser guardada no dispositivo para reconexão automática.
- O HTTPS é validado pela raiz pública ISRG Root X1 da Let's Encrypt.
- A opção para aceitar certificado não reconhecido deve permanecer desativada em produção.
- O portal usa uma senha individual derivada do identificador físico do chip e é desligado após a configuração.
- Apagar a configuração remove Wi-Fi, e-mail, sessão e preferências do namespace do firmware.

O NVS comum não é criptografado automaticamente em todas as placas. Para instalações com risco de acesso físico, habilite Flash Encryption e Secure Boot conforme a documentação da Espressif.

## Compilar e gravar com PlatformIO

O projeto padrão usa `esp32dev`, adequado ao ESP32 clássico:

```powershell
platformio run
platformio run --target upload --upload-port COM3
platformio device monitor --port COM3 --baud 115200
```

Para um ESP32-S3 DevKitC, selecione o ambiente correspondente:

```powershell
platformio run -e esp32-s3-devkitc-1
```

As dependências são instaladas automaticamente pelo PlatformIO:

- ArduinoJson 7
- Adafruit NeoPixel
- Bibliotecas nativas do Arduino ESP32 para Wi-Fi, HTTPS, DNS, NVS e servidor web

## Download oficial

As versões estáveis são publicadas em [GitHub Releases](https://github.com/RonaldoSobrinhoEB/rejoinbi-iot-esp32/releases). Baixe o arquivo `RejoinBI-IOT-ESP32-v1.0.zip`, extraia a pasta e abra o projeto no PlatformIO.

## Estrutura

```text
IOT/
├── .github/workflows/build.yml
├── CHANGELOG.md
├── platformio.ini
├── README.md
└── src/
    ├── main.cpp
    ├── icon_data.h
    ├── portal_page.h
    └── root_ca.h
```

## Referências técnicas

- [Arduino ESP32, documentação oficial](https://docs.espressif.com/projects/arduino-esp32/en/latest/)
- [Controle de LED no Arduino ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/basic.html)
- [Certificados oficiais da Let's Encrypt](https://letsencrypt.org/certificates/)
- [Segurança do ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/security.html)
