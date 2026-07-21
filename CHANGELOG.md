# Changelog

## v1.0.2

- Sessão do ESP32 identificada explicitamente como cliente IoT.
- A plataforma mantém a sessão IoT ativa sem expiração por inatividade, preservando revogação administrativa.

## v1.0.1

- Senha da rede temporária simplificada para `RJ` + os 6 caracteres visíveis no final do SSID.
- Exemplo: `RejoinBI-IOT-34E3EC` usa a senha `RJ34E3EC`.

## v1.0

- Portal local responsivo para configuração do dispositivo.
- Pesquisa de redes Wi-Fi de 2,4 GHz com indicação de sinal e segurança.
- Retorno explícito para conexão bem-sucedida, senha recusada, rede ausente e tempo esgotado.
- Autenticação Rejoin BI com suporte a PIN e armazenamento somente da sessão autorizada.
- Verificação periódica da disponibilidade da plataforma.
- Modos para LED comum, dois LEDs ou RGB endereçável.
- Reabertura segura do portal pelo botão BOOT.
- Ícone Rejoin BI incorporado diretamente ao firmware.
