#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "app_state.h"
#include "weather_decode.h"
#include "cooperative_reader.h"

struct WeatherReaderClock {
  static uint32_t now() { return millis(); }
  static void pause() { vTaskDelay(1); }
};

using WeatherReader = CooperativeReader<WiFiClient, WeatherReaderClock>;
constexpr uint32_t WEATHER_BODY_TIMEOUT_MS = 30000;

// Let's Encrypt trust anchors: ISRG Root X1 plus ISRG Root X2 and the 2025
// "Gen Y" roots Root YE (ECDSA) and Root YR (RSA). Servers moved to YE1/YR1
// intermediates in 2026; trusting these roots directly shortens the chain
// the ESP32 has to verify (Bright Sky: 2 instead of 4 signatures) and keeps
// working once the cross-signatures to X1 are no longer served.
static const char LETS_ENCRYPT_ROOTS[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIICGzCCAaGgAwIBAgIQQdKd0XLq7qeAwSxs6S+HUjAKBggqhkjOPQQDAzBPMQsw
CQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2gg
R3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMjAeFw0yMDA5MDQwMDAwMDBaFw00
MDA5MTcxNjAwMDBaME8xCzAJBgNVBAYTAlVTMSkwJwYDVQQKEyBJbnRlcm5ldCBT
ZWN1cml0eSBSZXNlYXJjaCBHcm91cDEVMBMGA1UEAxMMSVNSRyBSb290IFgyMHYw
EAYHKoZIzj0CAQYFK4EEACIDYgAEzZvVn4CDCuwJSvMWSj5cz3es3mcFDR0HttwW
+1qLFNvicWDEukWVEYmO6gbf9yoWHKS5xcUy4APgHoIYOIvXRdgKam7mAHf7AlF9
ItgKbppbd9/w+kHsOdx1ymgHDB/qo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0T
AQH/BAUwAwEB/zAdBgNVHQ4EFgQUfEKWrt5LSDv6kviejM9ti6lyN5UwCgYIKoZI
zj0EAwMDaAAwZQIwe3lORlCEwkSHRhtFcP9Ymd70/aTSVaYgLXTWNLxBo1BfASdW
tL4ndQavEi51mI38AjEAi/V3bNTIZargCyzuFJ0nN6T5U6VR5CmD1/iQMVtCnwr1
/q4AaOeMSQ+2b1tbFfLn
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIB2TCCAWCgAwIBAgIRAKQCa6LvbHwg1AR+XmWmk4AwCgYIKoZIzj0EAwMwLjEL
MAkGA1UEBhMCVVMxDTALBgNVBAoTBElTUkcxEDAOBgNVBAMTB1Jvb3QgWUUwHhcN
MjUwOTAzMDAwMDAwWhcNNDUwOTAyMjM1OTU5WjAuMQswCQYDVQQGEwJVUzENMAsG
A1UEChMESVNSRzEQMA4GA1UEAxMHUm9vdCBZRTB2MBAGByqGSM49AgEGBSuBBAAi
A2IABDwS/6vhrcVqcbBo+wgdI3fwn9x7DNJJOY/lTOti0vkwuRN87RhEhTH17E7X
yFjWsPYhIPt/wzOqxTd2b+4ZJNy9ID04YywF9U5zasDVyGSNErVNtz8uSGh5izW8
7j77GaNCMEAwDgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQFMAMBAf8wHQYDVR0O
BBYEFKPIJlqOoUzQNWP8myPIOq5W809WMAoGCCqGSM49BAMDA2cAMGQCMHhMr8N9
LdL1VQKs9BdV81r76eXRB6mtjuNjzk6/lBsPNToWLTDzGYgtQKO1jl63uAIwGV7m
onyF377c+MM1oqVNs17sgu7F9YKZwgLmVbeOMDbKAXHtKMDLbiGllCcs8f47
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIFKTCCAxGgAwIBAgIRAOxGNJNgz0sP+KmC2Tqpyj0wDQYJKoZIhvcNAQELBQAw
LjELMAkGA1UEBhMCVVMxDTALBgNVBAoTBElTUkcxEDAOBgNVBAMTB1Jvb3QgWVIw
HhcNMjUwOTAzMDAwMDAwWhcNNDUwOTAyMjM1OTU5WjAuMQswCQYDVQQGEwJVUzEN
MAsGA1UEChMESVNSRzEQMA4GA1UEAxMHUm9vdCBZUjCCAiIwDQYJKoZIhvcNAQEB
BQADggIPADCCAgoCggIBANvGJnN78CTJdWL3+eGfsLN5TrNBJs+VH9hRXqRbwxu9
sGNiB0BD1fcOxbSUQCJIM1xE13Db+5Cw1w0s0EBYsvuIP/6joF0w8cuImbgR1OGg
YbSQ4OpzI+DG8SGuTlcE873OCS+kh3srlo6vl43M5OJg4Aeo1sfHp6kTJDoIiFBN
JAY+OKfX/FUvYKuhjT+no49lmqmupSBI5PkBQiqrEGtWU5uxU/cQWHGu8jSjFBzn
ZqvbNPLMXMLFxCb3WTfrJBXXjqvWG+v4bjzxjjeAtOlU7qarRDvNOyAuQYLln904
M+faKx8hnLCpJ15ZqaEgcNlY+9MMWcC5yvL2A2j3l9+2buggZX+dOE91zYmIdawT
vSZuVvlbRrAlLxIB6pwMBjneXCjYQ8+3BCCjssbSNpZU3hTcBDdhfAlEDlYr6pEa
tnMdmDT5BqnKC92bd0EhM1fbLHioLccLCuievT8ZkPhZrq7Mii7gNXAcUEAR8+lz
Yal+9zTg7C5DALyVOeG/CqfRAMn1KSHCR0NSA6P8tn/mGRlnCct5rtVCLnVySVpU
6H1qGg3DgTOuskf8eahTMiYbI5ezPJmO5ertalskQ1utp74+eDy92PI4ftHKTbq9
IWhH4YZKh3WnJEIt+oQvlYZbY8tpEroKrFB6PFGzrJIDRyts4HqvuH52RFj2zv/B
AgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8EBTADAQH/MB0GA1Ud
DgQWBBTe51tg0CJtQCh9Pw0B/qS1UrRRlDANBgkqhkiG9w0BAQsFAAOCAgEAWHnf
713Bdkq7t5yN2dNIgQakUb94X9WuyhMEHHkgx4oDpSUlnG0w4g94MoqaEUE31ZjR
LU7L5LD1g9ujFHTQu8AD215AHMVQFbm6j8hQxdXHAzDajFNQnOlDJrLjzIx176oy
AjvUtejZx2NNmdb5fd0WGVGsCdoAJ3N8ozo7ajE8t6vfxStZb4BQ9WYJGHUDrv2N
i5tJF6CNiPnlzs3BUfECRbE4JSk+jvy8+VoGiFE8qsH/j78x2fjgQhAQFV7P7Zxy
dBTZ1wEkNpZNW2qnaK1SKBLa+xf6E06YRIq5uaI+HWH8SY1y5VbRgzq40EKg3yxP
06fz+uYAUIFJoLNfhwRCc3Q6pQVuMX3yAjHAes4gk4moGcLQ5p7HAh39yeylZc1J
41sx/jKwLIkPE6Rr1Nf4pxdsxf9SA4yOEiAkDgq04DVxn8hgYFdUtBCuiuVC2heA
EiqVEa+8QZjuw8Gj0EbHXcRd1nInvGqRS1o9Is7YBdQN57X1AYveGBNNqjICSb7c
awuw1EawTDrs13VUlJVEsbQ0/O/1aaV73mCdOQ8azqL2KTv1Ewu1xbquE2S+kdQU
To9TUwat3wUA6cwXh1EfpS/3fJ0aGah5hdpRyoCLDlsSn8tkrjMfFFX0viC+GxHc
sI1ANRYvqSFC2X1VRZfDg+wD6E21BccmifG4yWc=
-----END CERTIFICATE-----
)EOF";

static const char SECTIGO_PUBLIC_SERVER_AUTH_ROOT_R46[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFijCCA3KgAwIBAgIQdY39i658BwD6qSWn4cetFDANBgkqhkiG9w0BAQwFADBf
MQswCQYDVQQGEwJHQjEYMBYGA1UEChMPU2VjdGlnbyBMaW1pdGVkMTYwNAYDVQQD
Ey1TZWN0aWdvIFB1YmxpYyBTZXJ2ZXIgQXV0aGVudGljYXRpb24gUm9vdCBSNDYw
HhcNMjEwMzIyMDAwMDAwWhcNNDYwMzIxMjM1OTU5WjBfMQswCQYDVQQGEwJHQjEY
MBYGA1UEChMPU2VjdGlnbyBMaW1pdGVkMTYwNAYDVQQDEy1TZWN0aWdvIFB1Ymxp
YyBTZXJ2ZXIgQXV0aGVudGljYXRpb24gUm9vdCBSNDYwggIiMA0GCSqGSIb3DQEB
AQUAA4ICDwAwggIKAoICAQCTvtU2UnXYASOgHEdCSe5jtrch/cSV1UgrJnwUUxDa
ef0rty2k1Cz66jLdScK5vQ9IPXtamFSvnl0xdE8H/FAh3aTPaE8bEmNtJZlMKpnz
SDBh+oF8HqcIStw+KxwfGExxqjWMrfhu6DtK2eWUAtaJhBOqbchPM8xQljeSM9xf
iOefVNlI8JhD1mb9nxc4Q8UBUQvX4yMPFF1bFOdLvt30yNoDN9HWOaEhUTCDsG3X
ME6WW5HwcCSrv0WBZEMNvSE6Lzzpng3LILVCJ8zab5vuZDCQOc2TZYEhMbUjUDM3
IuM47fgxMMxF/mL50V0yeUKH32rMVhlATc6qu/m1dkmU8Sf4kaWD5QazYw6A3OAS
VYCmO2a0OYctyPDQ0RTp5A1NDvZdV3LFOxxHVp3i1fuBYYzMTYCQNFu31xR13NgE
SJ/AwSiItOkcyqex8Va3e0lMWeUgFaiEAin6OJRpmkkGj80feRQXEgyDet4fsZfu
+Zd4KKTIRJLpfSYFplhym3kT2BFfrsU4YjRosoYwjviQYZ4ybPUHNs2iTG7sijbt
8uaZFURww3y8nDnAtOFr94MlI1fZEoDlSfB1D++N6xybVCi0ITz8fAr/73trdf+L
HaAZBav6+CuBQug4urv7qv094PPK306Xlynt8xhW6aWWrL3DkJiy4Pmi1KZHQ3xt
zwIDAQABo0IwQDAdBgNVHQ4EFgQUVnNYZJX5khqwEioEYnmhQBWIIUkwDgYDVR0P
AQH/BAQDAgGGMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEMBQADggIBAC9c
mTz8Bl6MlC5w6tIyMY208FHVvArzZJ8HXtXBc2hkeqK5Duj5XYUtqDdFqij0lgVQ
YKlJfp/imTYpE0RHap1VIDzYm/EDMrraQKFz6oOht0SmDpkBm+S8f74TlH7Kph52
gDY9hAaLMyZlbcp+nv4fjFg4exqDsQ+8FxG75gbMY/qB8oFM2gsQa6H61SilzwZA
Fv97fRheORKkU55+MkIQpiGRqRxOF3yEvJ+M0ejf5lG5Nkc/kLnHvALcWxxPDkjB
JYOcCj+esQMzEhonrPcibCTRAUH4WAP+JWgiH5paPHxsnnVI84HxZmduTILA7rpX
DhjvLpr3Etiga+kFpaHpaPi8TD8SHkXoUsCjvxInebnMMTzD9joiFgOgyY9mpFui
TdaBJQbpdqQACj7LzTWb4OE4y2BThihCQRxEV+ioratF4yUQvNs+ZUH7G6aXD+u5
dHn5HrwdVw1Hr8Mvn4dGp+smWg9WY7ViYG4A++MnESLn/pmPNPW56MORcr3Ywx65
LvKRRFHQV80MNNVIIb/bE/FmJUNS0nAiNs2fxBx1IK1jcmMGDw4nztJqDby1ORrp
0XZ60Vzk50lJLVU3aPAaOpg+VBeHVOmmJ1CJeyAvP/+/oYtKR5j/K3tJPsMpRmAY
QqszKbrAKbkTidOIijlBO8n9pu0f9GBj39ItVQGL
-----END CERTIFICATE-----
)EOF";

static const char HARICA_ROOT[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIGCzCCA/OgAwIBAgIBADANBgkqhkiG9w0BAQsFADCBpjELMAkGA1UEBhMCR1IxDzANBgNVBAcT
BkF0aGVuczFEMEIGA1UEChM7SGVsbGVuaWMgQWNhZGVtaWMgYW5kIFJlc2VhcmNoIEluc3RpdHV0
aW9ucyBDZXJ0LiBBdXRob3JpdHkxQDA+BgNVBAMTN0hlbGxlbmljIEFjYWRlbWljIGFuZCBSZXNl
YXJjaCBJbnN0aXR1dGlvbnMgUm9vdENBIDIwMTUwHhcNMTUwNzA3MTAxMTIxWhcNNDAwNjMwMTAx
MTIxWjCBpjELMAkGA1UEBhMCR1IxDzANBgNVBAcTBkF0aGVuczFEMEIGA1UEChM7SGVsbGVuaWMg
QWNhZGVtaWMgYW5kIFJlc2VhcmNoIEluc3RpdHV0aW9ucyBDZXJ0LiBBdXRob3JpdHkxQDA+BgNV
BAMTN0hlbGxlbmljIEFjYWRlbWljIGFuZCBSZXNlYXJjaCBJbnN0aXR1dGlvbnMgUm9vdENBIDIw
MTUwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQDC+Kk/G4n8PDwEXT2QNrCROnk8Zlrv
bTkBSRq0t89/TSNTt5AA4xMqKKYx8ZEA4yjsriFBzh/a/X0SWwGDD7mwX5nh8hKDgE0GPt+sr+eh
iGsxr/CL0BgzuNtFajT0AoAkKAoCFZVedioNmToUW/bLy1O8E00BiDeUJRtCvCLYjqOWXjrZMts+
6PAQZe104S+nfK8nNLspfZu2zwnI5dMK/IhlZXQK3HMcXM1AsRzUtoSMTFDPaI6oWa7CJ06CojXd
FPQf/7J31Ycvqm59JCfnxssm5uX+Zwdj2EUN3TpZZTlYepKZcj2chF6IIbjV9Cz82XBST3i4vTwr
i5WY9bPRaM8gFH5MXF/ni+X1NYEZN9cRCLdmvtNKzoNXADrDgfgXy5I2XdGj2HUb4Ysn6npIQf1F
GQatJ5lOwXBH3bWfgVMS5bGMSF0xQxfjjMZ6Y5ZLKTBOhE5iGV48zpeQpX8B653g+IuJ3SWYPZK2
fu/Z8VFRfS0myGlZYeCsargqNhEEelC9MoS+L9xy1dcdFkfkR2YgP/SWxa+OAXqlD3pk9Q0Yh9mu
iNX6hME6wGkoLfINaFGq46V3xqSQDqE3izEjR8EJCOtu93ib14L8hCCZSRm2Ekax+0VVFqmjZayc
Bw/qa9wfLgZy7IaIEuQt218FL+TwA9MmM+eAws1CoRc0CwIDAQABo0IwQDAPBgNVHRMBAf8EBTAD
AQH/MA4GA1UdDwEB/wQEAwIBBjAdBgNVHQ4EFgQUcRVnyMjJvXVdctA4GGqd83EkVAswDQYJKoZI
hvcNAQELBQADggIBAHW7bVRLqhBYRjTyYtcWNl0IXtVsyIe9tC5G8jH4fOpCtZMWVdyhDBKg2mF+
D1hYc2Ryx+hFjtyp8iY/xnmMsVMIM4GwVhO+5lFc2JsKT0ucVlMC6U/2DWDqTUJV6HwbISHTGzrM
d/K4kPFox/la/vot9L/J9UUbzjgQKjeKeaO04wlshYaT/4mWJ3iBj2fjRnRUjtkNaeJK9E10A/+y
d+2VZ5fkscWrv2oj6NSU4kQoYsRL4vDY4ilrGnB+JGGTe08DMiUNRSQrlrRGar9KC/eaj8GsGsVn
82800vpzY4zvFrCopEYq+OsS7HK07/grfoxSwIuEVPkvPuNVqNxmsdnhX9izjFk0WaSrT2y7Hxjb
davYy5LNlDhhDgcGH0tGEPEVvo2FXDtKK4F5D7Rpn0lQl033DlZdwJVqwjbDG2jJ9SrcR5q+ss7F
Jej6A7na+RZukYT1HCjI/CbM1xyQVqdfbzoEvM14iQuODy+jqk+iGxI9FghAD/FGTNeqewjBCvVt
J94Cj8rDtSvK6evIIVM4pcw72Hc3MKJP2W/R8kCtQXoXxdZKNYm3QdV8hn9VTYNKpXMgwDqvkPGa
JI7ZjnHKe7iG2rKPmT4dEw0SEe7Uq/DpFXYC5ODfqiAeW2GFZECpkJcNrVPSWh2HagCXZWK0vm9q
p/UsQu0yrbYhnr68
-----END CERTIFICATE-----
)EOF";

String urlEncode(const String &input) {
  const char *hex = "0123456789ABCDEF";
  String out;
  out.reserve(input.length() * 3);
  for (uint16_t i = 0; i < input.length(); i++) {
    const uint8_t c = input[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.') {
      out += char(c);
    } else if (c == ' ') {
      out += '+';
    } else {
      out += '%';
      out += hex[c >> 4];
      out += hex[c & 0x0f];
    }
  }
  return out;
}

String timezoneFromIana(const String &iana) {
  if (iana == "Etc/UTC" || iana == "UTC" || iana == "GMT") return "UTC0";

  if (iana == "Europe/Berlin" || iana == "Europe/Amsterdam" || iana == "Europe/Paris" ||
      iana == "Europe/Rome" || iana == "Europe/Madrid" || iana == "Europe/Brussels" ||
      iana == "Europe/Vienna" || iana == "Europe/Zurich" || iana == "Europe/Prague" ||
      iana == "Europe/Warsaw" || iana == "Europe/Stockholm" || iana == "Europe/Oslo" ||
      iana == "Europe/Copenhagen") {
    return "CET-1CEST,M3.5.0,M10.5.0/3";
  }
  if (iana == "Europe/London") return "GMT0BST,M3.5.0/1,M10.5.0";
  if (iana == "Europe/Lisbon") return "WET0WEST,M3.5.0/1,M10.5.0";
  if (iana == "Europe/Athens" || iana == "Europe/Helsinki" || iana == "Europe/Bucharest" ||
      iana == "Europe/Sofia" || iana == "Europe/Tallinn" || iana == "Europe/Riga" ||
      iana == "Europe/Vilnius") {
    return "EET-2EEST,M3.5.0/3,M10.5.0/4";
  }
  if (iana == "Europe/Moscow") return "MSK-3";

  if (iana == "America/New_York" || iana == "America/Toronto") return "EST5EDT,M3.2.0/2,M11.1.0/2";
  if (iana == "America/Chicago") return "CST6CDT,M3.2.0/2,M11.1.0/2";
  if (iana == "America/Denver") return "MST7MDT,M3.2.0/2,M11.1.0/2";
  if (iana == "America/Los_Angeles" || iana == "America/Vancouver") return "PST8PDT,M3.2.0/2,M11.1.0/2";
  if (iana == "America/Phoenix") return "MST7";
  if (iana == "America/Anchorage") return "AKST9AKDT,M3.2.0/2,M11.1.0/2";
  if (iana == "Pacific/Honolulu") return "HST10";
  if (iana == "America/Sao_Paulo") return "BRT3";

  if (iana == "Asia/Tokyo") return "JST-9";
  if (iana == "Asia/Shanghai" || iana == "Asia/Hong_Kong") return "CST-8";
  if (iana == "Asia/Singapore") return "SGT-8";
  if (iana == "Asia/Dubai") return "GST-4";
  if (iana == "Asia/Kolkata") return "IST-5:30";

  if (iana == "Australia/Sydney" || iana == "Australia/Melbourne") return "AEST-10AEDT,M10.1.0,M4.1.0/3";
  if (iana == "Australia/Brisbane") return "AEST-10";
  if (iana == "Australia/Perth") return "AWST-8";

  return "";
}

void configureWeatherClient(WiFiClientSecure &client, uint8_t provider) {
  client.setCACert(provider == WEATHER_PROVIDER_MET_NORWAY ? HARICA_ROOT :
    provider == WEATHER_PROVIDER_OPEN_WEATHER_MAP ? SECTIGO_PUBLIC_SERVER_AUTH_ROOT_R46 : LETS_ENCRYPT_ROOTS);
  // ECDSA P-384 chains are verified in software; leave room on a busy core.
  client.setHandshakeTimeout(12);
  // WiFiClientSecure uses seconds; HTTPClient and Stream use milliseconds.
  client.setTimeout((HTTP_TIMEOUT_MS + 999) / 1000);
}

static void prepareHttp(HTTPClient &http) {
  http.setConnectTimeout(HTTP_TIMEOUT_MS);
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.useHTTP10(true); // Stream JSON without chunk framing or a second body buffer.
  http.setUserAgent("PixelClock/" FIRMWARE_VERSION_TEXT " https://github.com/mBlinkii/PixelClock");
}

// Negative HTTPClient codes mean the connection or TLS handshake failed; the
// mbedtls text tells certificate problems apart from network timeouts.
static String connectionError(WiFiClientSecure &client, int code, const char *prefix) {
  char tlsError[96] = {};
  if (client.lastError(tlsError, sizeof(tlsError)) != 0 && tlsError[0]) {
    return String(prefix) + "TLS: " + tlsError;
  }
  return String(prefix) + "Verbindung fehlgeschlagen (" + HTTPClient::errorToString(code) + ")";
}

// Date and Expires use the same GMT clock. Only their difference is needed.
static uint32_t responseDelayMs(HTTPClient &http, const char *header) {
  struct tm expires = {}, date = {};
  if (!strptime(http.header(header).c_str(), "%a, %d %b %Y %H:%M:%S", &expires) ||
      !strptime(http.header("Date").c_str(), "%a, %d %b %Y %H:%M:%S", &date)) return 0;
  const double seconds = difftime(mktime(&expires), mktime(&date));
  return seconds > 0 ? static_cast<uint32_t>(min(seconds, 86400.0)) * 1000UL : 0;
}

static bool requestWeather(const AppConfig &source, WeatherState &sample) {
  if (source.weatherProvider == WEATHER_PROVIDER_OPEN_WEATHER_MAP && source.openWeatherApiKey.isEmpty()) {
    sample.lastError = "OpenWeatherMap API-Key fehlt";
    return false;
  }
  if (source.weatherProvider == WEATHER_PROVIDER_WEATHER_API && source.weatherApiKey.isEmpty()) {
    sample.lastError = "WeatherAPI API-Key fehlt";
    return false;
  }
  String url;
  url.reserve(320);
  if (source.weatherProvider == WEATHER_PROVIDER_OPEN_WEATHER_MAP) {
    url = "https://api.openweathermap.org/data/2.5/weather?lat=" + String(source.latitude, 4) +
      "&lon=" + String(source.longitude, 4) + "&appid=" + urlEncode(source.openWeatherApiKey) + "&units=metric";
  } else if (source.weatherProvider == WEATHER_PROVIDER_DWD) {
    url = "https://api.brightsky.dev/current_weather?lat=" + String(source.latitude, 4) + "&lon=" + String(source.longitude, 4);
  } else if (source.weatherProvider == WEATHER_PROVIDER_MET_NORWAY) {
    url = "https://api.met.no/weatherapi/locationforecast/2.0/compact?lat=" + String(source.latitude, 4) + "&lon=" + String(source.longitude, 4);
  } else if (source.weatherProvider == WEATHER_PROVIDER_WEATHER_API) {
    url = "https://api.weatherapi.com/v1/forecast.json?key=" + urlEncode(source.weatherApiKey) +
      "&q=" + String(source.latitude, 4) + "," + String(source.longitude, 4) +
      "&days=1&aqi=no&alerts=no&hour=0";
  } else {
    url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(source.latitude, 4) +
      "&longitude=" + String(source.longitude, 4) +
      "&current=temperature_2m,weather_code,is_day&daily=temperature_2m_max,temperature_2m_min&timezone=auto&forecast_days=1";
  }
  WiFiClientSecure client;
  configureWeatherClient(client, source.weatherProvider);
  HTTPClient http;
  prepareHttp(http);
  if (!http.begin(client, url)) {
    sample.lastError = "HTTP begin failed";
    return false;
  }
  const char *headers[] = {"Date", "Expires", "Last-Modified", "Retry-After"};
  http.collectHeaders(headers, 4);
  http.addHeader("Accept-Encoding", "identity");
  if (source.weatherProvider == WEATHER_PROVIDER_MET_NORWAY && !sample.lastModified.isEmpty()) {
    http.addHeader("If-Modified-Since", sample.lastModified);
  }
  const int code = http.GET();
  if (source.weatherProvider == WEATHER_PROVIDER_MET_NORWAY) {
    const uint32_t cacheMs = responseDelayMs(http, "Expires");
    sample.cacheUntil = millis() + max(cacheMs, WEATHER_RETRY_MS);
  }
  if (source.weatherProvider == WEATHER_PROVIDER_MET_NORWAY && code == HTTP_CODE_NOT_MODIFIED && isfinite(sample.temperature)) {
    http.end();
    return true;
  }
  if (code < 0) {
    sample.lastError = connectionError(client, code, "");
    http.end();
    return false;
  }
  if (code != HTTP_CODE_OK) {
    sample.lastError = "HTTP " + String(code);
    const long retrySeconds = http.header("Retry-After").toInt();
    if (retrySeconds > 0) sample.retryAfterMs = max<uint32_t>(WEATHER_RETRY_MS, static_cast<uint32_t>(min(retrySeconds, 86400L)) * 1000UL);
    else sample.retryAfterMs = max(sample.retryAfterMs, responseDelayMs(http, "Retry-After"));
    if (code == 429) sample.retryAfterMs = max<uint32_t>(sample.retryAfterMs, 30UL * 60UL * 1000UL);
    http.end();
    return false;
  }
  const String modified = http.header("Last-Modified");
  JsonDocument doc, filter;
  filterWeather(filter, source.weatherProvider);
  WeatherReader stream(http.getStream(), HTTP_TIMEOUT_MS, WEATHER_BODY_TIMEOUT_MS);
  // Only the nearest forecast is displayed. Do not allocate the entire MET timeseries.
  if (source.weatherProvider == WEATHER_PROVIDER_MET_NORWAY &&
      (!stream.find("\"timeseries\"") || !stream.find("["))) {
    sample.lastError = "Ungültige Wetterdaten";
    http.end();
    return false;
  }
  const DeserializationError error = deserializeJson(doc, stream, DeserializationOption::Filter(filter), DeserializationOption::NestingLimit(16));
  http.end();
  if (error) {
    sample.lastError = error.c_str();
    return false;
  }
  decodeWeather(doc, source.weatherProvider, sample);
  if (!isfinite(sample.temperature)) {
    sample.lastError = "Ungültige Wetterdaten";
    return false;
  }
  sample.lastModified = modified;
  return true;
}

void fetchWeather() {
  AppConfig source;
  WeatherState sample;
  uint32_t revision;
  const uint32_t started = millis();
  {
    StateLock lock;
    if (WiFi.status() != WL_CONNECTED || pendingCityResolve || pendingRestart) return;
    source = config;
    sample = weather;
    revision = weatherRevision;
    weather.busy = true;
    weather.lastAttempt = started;
    pendingWeatherFetch = false;
  }
  sample.retryAfterMs = WEATHER_RETRY_MS;
  const bool success = requestWeather(source, sample);
  StateLock lock;
  weather.busy = false;
  if (revision != weatherRevision || pendingRestart) return; // Ignore obsolete responses.
  weather.fetchDurationMs = millis() - started;
  weather.cacheUntil = sample.cacheUntil;
  weather.retryAfterMs = sample.retryAfterMs;
  if (!success) {
    weather.lastError = sample.lastError;
    return; // Keep the last valid sample and its age on all errors.
  }
  weather.temperature = sample.temperature;
  weather.temperatureMin = sample.temperatureMin;
  weather.temperatureMax = sample.temperatureMax;
  weather.weatherCode = sample.weatherCode;
  weather.isDay = sample.isDay;
  weather.lastModified = sample.lastModified;
  weather.lastFetch = millis();
  weather.lastError = "";
  lastRender = 0;
}

bool resolveCity() {
  AppConfig source;
  uint32_t revision;
  {
    StateLock lock;
    if (WiFi.status() != WL_CONNECTED || config.cityName.length() < 2) return false;
    source = config;
    revision = weatherRevision;
  }
  WiFiClientSecure client;
  configureWeatherClient(client, WEATHER_PROVIDER_OPEN_METEO);
  HTTPClient http;
  prepareHttp(http);
  const String url = "https://geocoding-api.open-meteo.com/v1/search?name=" + urlEncode(source.cityName) +
    "&count=1&language=" + source.language + "&format=json";
  String error;
  JsonDocument doc, filter;
  for (const char *key : {"latitude", "longitude", "name", "country", "timezone"}) filter["results"][0][key] = true;
  if (!http.begin(client, url)) error = "Geocoding begin failed";
  else {
    http.addHeader("Accept-Encoding", "identity");
    const int code = http.GET();
    if (code < 0) error = connectionError(client, code, "Geocoding ");
    else if (code != HTTP_CODE_OK) error = "Geocoding HTTP " + String(code);
    else {
      WeatherReader stream(http.getStream(), HTTP_TIMEOUT_MS, WEATHER_BODY_TIMEOUT_MS);
      const DeserializationError err = deserializeJson(doc, stream, DeserializationOption::Filter(filter), DeserializationOption::NestingLimit(16));
      if (err) error = String("Geocoding JSON ") + err.c_str();
    }
    http.end();
  }
  JsonObject first = doc["results"][0];
  const float latitude = first["latitude"] | NAN;
  const float longitude = first["longitude"] | NAN;
  if (error.isEmpty() && (!isfinite(latitude) || !isfinite(longitude))) error = "Stadt nicht gefunden";
  StateLock lock;
  if (revision != weatherRevision || pendingRestart) return false;
  if (!error.isEmpty()) {
    weather.lastError = error;
    return false;
  }
  config.latitude = latitude;
  config.longitude = longitude;
  config.resolvedCityName = source.cityName;
  config.locationLabel = first["name"] | source.cityName.c_str();
  const char *country = first["country"] | "";
  if (*country) config.locationLabel += String(", ") + country;
  // Preserve a timezone explicitly edited while geocoding was in flight.
  const String timezone = timezoneFromIana(first["timezone"] | "");
  if (!timezone.isEmpty() && config.timezone == source.timezone) config.timezone = timezone;
  weather = WeatherState();
  ++weatherRevision;
  pendingCityResolve = false;
  pendingTimeSync = true;
  pendingWeatherFetch = true;
  saveConfig();
  return true;
}
