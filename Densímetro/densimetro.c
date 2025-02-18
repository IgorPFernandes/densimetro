#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "hardware/adc.h"
#include "Servo.h"
#include "ultrasonic.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwipopts.h"
#include "hardware/pwm.h"

#define I2C_PORT i2c1
#define PINO_SCL 14
#define PINO_SDA 15
#define JOYSTICK_X_PIN 26
#define BUTTON_A_PIN 5
#define BUTTON_B_PIN 6
#define TRIGGER_PIN 17
#define ECHO_PIN 19
#define LED_R_PIN 13
#define LED_G_PIN 11
#define LED_B_PIN 12
#define WIFI_SSID "Casa" // Rede
#define WIFI_PASS "a1a2a3a4a5" // Senha
#define THINGSPEAK_API_KEY "TZTLPTFS9YTYGQXX" // API-KEY THINGSPEAK
#define THINGSPEAK_SERVER "api.thingspeak.com" // Host
#define THINGSPEAK_PORT 80 // Porta padrão HTTP, lembrei da aula de Moroni "Quem lembra qual a porta ?"

ssd1306_t disp;
float distancia_global = 0.0, peso_global = 120.0; 

//Como não podia falta, uma marca registrada da embarcatech, um semáforo!! KKKKKKKKK
// Incorporei um semáforo de liberação de envio e aviso de resposta (Verde/Vermelho), também implementei o amarelo para informar se tem conexão wi-fi
void set_led_color(bool r, bool g, bool b) {
    pwm_set_chan_level(pwm_gpio_to_slice_num(LED_R_PIN), pwm_gpio_to_channel(LED_R_PIN), r ? 25 : 0); //cor_do_led
    pwm_set_chan_level(pwm_gpio_to_slice_num(LED_G_PIN), pwm_gpio_to_channel(LED_G_PIN), g ? 25 : 0); //cor_do_led
    pwm_set_chan_level(pwm_gpio_to_slice_num(LED_B_PIN), pwm_gpio_to_channel(LED_B_PIN), b ? 25 : 0); //cor_do_led
}

void inicializa() {
    stdio_init_all();
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(PINO_SCL, GPIO_FUNC_I2C);
    gpio_set_function(PINO_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(PINO_SCL);
    gpio_pull_up(PINO_SDA);
    ssd1306_init(&disp, 128, 64, 0x3C, I2C_PORT);
    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);
    ultrasonic_init(TRIGGER_PIN, ECHO_PIN);

    gpio_set_function(LED_R_PIN, GPIO_FUNC_PWM);
    gpio_set_function(LED_G_PIN, GPIO_FUNC_PWM);
    gpio_set_function(LED_B_PIN, GPIO_FUNC_PWM);

    pwm_set_wrap(pwm_gpio_to_slice_num(LED_R_PIN), 255);
    pwm_set_wrap(pwm_gpio_to_slice_num(LED_G_PIN), 255);
    pwm_set_wrap(pwm_gpio_to_slice_num(LED_B_PIN), 255);

    pwm_set_chan_level(pwm_gpio_to_slice_num(LED_R_PIN), pwm_gpio_to_channel(LED_R_PIN), 25); //cor_do_led
    pwm_set_chan_level(pwm_gpio_to_slice_num(LED_G_PIN), pwm_gpio_to_channel(LED_G_PIN), 25); //cor_do_led
    pwm_set_chan_level(pwm_gpio_to_slice_num(LED_B_PIN), pwm_gpio_to_channel(LED_B_PIN), 25); //cor_do_led

    pwm_set_enabled(pwm_gpio_to_slice_num(LED_R_PIN), true);
    pwm_set_enabled(pwm_gpio_to_slice_num(LED_G_PIN), true);
    pwm_set_enabled(pwm_gpio_to_slice_num(LED_B_PIN), true);

    set_led_color(false, true, false); //Verde
}

/*Vocês poderiam perguntar, igor porque o display oled se você envia para o thingspeak que mostra as informações ?
1º - Que não seria dinâmico com o aluno
2º - Não poderia ser utilizado em uma prova ou quando a universidade estivesse sem internet
*/

void print_texto(char *msg, uint pos_x, uint pos_y) {
    ssd1306_draw_string(&disp, pos_x, pos_y, 1, msg);
    ssd1306_show(&disp);
}

void inicializa_display() {
    ssd1306_clear(&disp);
    print_texto("Dados dos sensores", 0, 2);
    print_texto("Distancia | Peso", 0, 15);
}

void atualiza_display(float distancia, float peso, uint linha) {
    char texto[32];
    snprintf(texto, sizeof(texto), "%.2f cm | %.2f N", distancia, peso);
    print_texto(texto, 0, 30 + 10 * linha);
}
/* Atenção a essa função!!! (err_t tcp_received_callback)
Nessa função, p->len imprime o comprimento da resposta do servidor. Se a resposta do servidor é recebida corretamente, p->len contém um número maior que 0.
Caso contrário, quando p é NULL ou ocorre um erro, a mensagem de erro e o valor err são impressos, sendo err 0 em caso de erro. Fiquem atentos ao monitor serial,
ele vai informar se deu certo ou não, o led é um indicativo de envio e uma afirmação de liberação, mas não uma certeza de que foi enviado!! Apenas a mensagem
no moitor serial vai garantir que a mensagem enviada foi recebida pelo thingspeak.
*/
static err_t tcp_received_callback(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    if (err != ERR_OK || p == NULL) {
        printf("Erro ao receber resposta do servidor: %d\n", err); // depuração
        if (p) pbuf_free(p);
        tcp_close(pcb);
        set_led_color(false, true, false); //Verde
        return err;
    }
    printf("Resposta recebida do servidor: %.*s\n", p->len, (char*)p->payload); // depuração
    pbuf_free(p);
    tcp_close(pcb);
    set_led_color(false, true, false); //Verde
    return ERR_OK;
}

static err_t tcp_connected_callback(void *arg, struct tcp_pcb *pcb, err_t err) {
    if (err != ERR_OK) {
        printf("Erro na conexão TCP: %d\n", err); // depuração
        set_led_color(true, false, false); //Vermelho
        return err;
    }
    printf("Enviando dados para ThingSpeak: %.2f cm, %.2f N\n", distancia_global, peso_global); // depuração

    char http_request[256];
    int len = snprintf(http_request, sizeof(http_request),
            "GET /update?api_key=%s&field1=%.2f&field2=%.2f HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Connection: close\r\n\r\n",
            THINGSPEAK_API_KEY, distancia_global, peso_global, THINGSPEAK_SERVER);

    if (len < 0 || len >= sizeof(http_request)) {
        printf("Erro ao construir a requisição HTTP\n"); // depuração
        set_led_color(true, false, false); //Vermelho
        return ERR_BUF;
    }
    printf("Requisição HTTP: %s\n", http_request); // depuração

    err = tcp_write(pcb, http_request, strlen(http_request), TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("Erro ao enviar requisição HTTP: %d\n", err); // depuração
        set_led_color(true, false, false); //Vermelho
        return err;
    }

    tcp_output(pcb);
    tcp_recv(pcb, tcp_received_callback);
    set_led_color(true, false, false); //Vermelho
    return ERR_OK;
}

static err_t tcp_connected_callback_b(void *arg, struct tcp_pcb *pcb, err_t err) {
    if (err != ERR_OK) {
        printf("Erro na conexão TCP: %d\n", err); // depuração
        set_led_color(true, false, false); //Vermelho
        return err;
    }
    printf("Enviando dados para ThingSpeak: %.2f cm, %.2f N\n", distancia_global, peso_global); // depuração

    char http_request[256];
    int len = snprintf(http_request, sizeof(http_request),
            "GET /update?api_key=%s&field3=%.2f&field4=%.2f HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Connection: close\r\n\r\n",
            THINGSPEAK_API_KEY, distancia_global, peso_global, THINGSPEAK_SERVER);

    if (len < 0 || len >= sizeof(http_request)) {
        printf("Erro ao construir a requisição HTTP\n"); // depuração
        set_led_color(true, false, false); //Vermelho
        return ERR_BUF;
    }
    printf("Requisição HTTP: %s\n", http_request); // depuração

    err = tcp_write(pcb, http_request, strlen(http_request), TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("Erro ao enviar requisição HTTP: %d\n", err); // depuração
        set_led_color(true, false, false); //Vermelho
        return err;
    }

    tcp_output(pcb);
    tcp_recv(pcb, tcp_received_callback);
    set_led_color(true, false, false); //Vermelho
    return ERR_OK;
}
// Informa caso tenha problema para se conectar com o thingspeak
static void dns_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    if (!ipaddr) {
        printf("Erro ao resolver IP do servidor ThingSpeak\n"); // depuração
        set_led_color(true, false, false); //Vermelho
        return;
    }
    printf("IP do servidor ThingSpeak resolvido: %s\n", ipaddr_ntoa(ipaddr)); // depuração
    struct tcp_pcb *pcb = (struct tcp_pcb *)callback_arg;
    tcp_connect(pcb, ipaddr, THINGSPEAK_PORT, tcp_connected_callback);
}

static void dns_callback_b(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    if (!ipaddr) {
        printf("Erro ao resolver IP do servidor ThingSpeak\n"); // depuração
        set_led_color(true, false, false); //Vermelho
        return;
    }
    printf("IP do servidor ThingSpeak resolvido: %s\n", ipaddr_ntoa(ipaddr)); // depuração
    struct tcp_pcb *pcb = (struct tcp_pcb *)callback_arg;
    tcp_connect(pcb, ipaddr, THINGSPEAK_PORT, tcp_connected_callback_b);
}
// Envia dados para o thingspeak
static void send_to_thingspeak() {
    struct tcp_pcb *pcb = tcp_new();
    ip_addr_t server_ip;
    if (!pcb) {
        printf("Erro ao criar PCB\n"); // depuração
        set_led_color(true, false, false); //Vermelho
        return;
    }

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Falha ao conectar ao Wi-Fi\n"); // depuração
        tcp_close(pcb);
        set_led_color(true, true, false); //Amarelo
        return;
    }
    printf("Wi-Fi reconectado\n"); // depuração

    err_t err = dns_gethostbyname(THINGSPEAK_SERVER, &server_ip, dns_callback, pcb);
    if (err == ERR_INPROGRESS) {
        printf("Resolução do DNS em andamento...\n"); // depuração
    } else if (err == ERR_OK) {
        dns_callback(THINGSPEAK_SERVER, &server_ip, pcb);
    } else {
        printf("Erro ao iniciar a resolução do DNS: %d\n", err); // depuração
        tcp_close(pcb);
        set_led_color(true, false, false); //Vermelho
    }
}

static void send_to_thingspeak_b() {
    struct tcp_pcb *pcb = tcp_new();
    ip_addr_t server_ip;
    if (!pcb) {
        printf("Erro ao criar PCB\n"); // depuração
        set_led_color(true, false, false); //Vermelho
        return;
    }

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Falha ao conectar ao Wi-Fi\n"); // depuração
        tcp_close(pcb);
        set_led_color(true, true, false); //Amarelo
        return;
    }
    printf("Wi-Fi reconectado\n"); // depuração

    err_t err = dns_gethostbyname(THINGSPEAK_SERVER, &server_ip, dns_callback_b, pcb);
    if (err == ERR_INPROGRESS) {
        printf("Resolução do DNS em andamento...\n"); // depuração
    } else if (err == ERR_OK) {
        dns_callback_b(THINGSPEAK_SERVER, &server_ip, pcb);
    } else {
        printf("Erro ao iniciar a resolução do DNS: %d\n", err); // depuração
        tcp_close(pcb);
        set_led_color(true, false, false); //Vermelho
    }
}

int main() {
    inicializa();
    inicializa_display();

    if (cyw43_arch_init()) {
        printf("Erro ao inicializar o Wi-Fi\n"); // depuração
        return -1;
    }
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000) == 0) {
        uint8_t *ip_address = (uint8_t *)&(cyw43_state.netif[0].ip_addr.addr);
        printf("Wi-Fi conectado. Endereço IP %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]); // depuração
    } else {
        printf("Falha ao conectar ao Wi-Fi\n"); // depuração
        set_led_color(true, true, false); //Amarelo
    }

    Servo* servo = servo_create(8);
    if (!servo) return 1;

    bool button_a_was_pressed = false, button_b_was_pressed = false;
    uint linha = 0;

    while (1) { //While com Temporizador
        bool button_a_is_pressed = (gpio_get(BUTTON_A_PIN) == 0);
        bool button_b_is_pressed = (gpio_get(BUTTON_B_PIN) == 0);

        if (button_a_is_pressed && !button_a_was_pressed) {
            distancia_global = measure_distance();
            printf("Leitura do sensor: %.2f cm\n", distancia_global); // depuração
            set_led_color(true, false, false); // vermelho
            send_to_thingspeak();
            if (linha > 2) {
                inicializa_display();
                linha = 0;
            }
            atualiza_display(distancia_global, peso_global, linha);
            linha++;
        }

        if (button_b_is_pressed && !button_b_was_pressed) {
            distancia_global = measure_distance();
            printf("Leitura do sensor: %.2f cm\n", distancia_global); // depuração
            set_led_color(true, false, false); // Vermelho
            send_to_thingspeak_b();
            if (linha > 2) {
                inicializa_display();
                linha = 0;
            }
            atualiza_display(distancia_global, peso_global, linha);
            linha++;
        }

        button_a_was_pressed = button_a_is_pressed;
        button_b_was_pressed = button_b_is_pressed;

        adc_select_input(0);
        servo_go_degree(servo, adc_read() * 180 / 4095);
        sleep_ms(50); 
        /*Esse atraso limita a taxa de atualização do programa, garantindo que as operações (como leitura do joystick e controle do servo) não sejam executadas continuamente sem pausa*/
    }
    servo_destroy(servo);
    return 0;
}
// Desde já, fico muito agradecido pelo tempo e a paciência de analisar meu código com calma, obrigado!!