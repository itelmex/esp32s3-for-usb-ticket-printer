#include "usb/usb_host.h"

usb_host_client_handle_t client_hdl;
usb_device_handle_t dev_hdl = NULL;
uint8_t ticket[] = {0x1B, 0x40, 'P', 'O', 'R', ' ', 'F', 'I', 'N', '!', '\n', 0x0A, 0x0A, 0x0A};

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n--- RECLAMANDO INTERFAZ DE IMPRESORA ---");

  usb_host_config_t host_config = { .intr_flags = ESP_INTR_FLAG_LEVEL1 };
  usb_host_install(&host_config);

  usb_host_client_config_t client_config = {
    .is_synchronous = true,
    .max_num_event_msg = 5,
    .async = { .client_event_callback = [](const usb_host_client_event_msg_t *event, void *arg) {
        if (event->event == USB_HOST_CLIENT_EVENT_NEW_DEV) {
          usb_host_device_open(client_hdl, event->new_dev.address, &dev_hdl);
        }
    }, .callback_arg = NULL }
  };
  usb_host_client_register(&client_config, &client_hdl);
}

void loop() {
  uint32_t event_flags;
  usb_host_lib_handle_events(1, &event_flags);

  if (dev_hdl != NULL) {
    static bool proceso_completo = false;
    if (!proceso_completo) {
      // PASO CRÍTICO: Reclamar la interfaz 0 (la mayoría de impresoras solo tienen una)
      esp_err_t err = usb_host_interface_claim(client_hdl, dev_hdl, 0, 0);
      
      if (err == ESP_OK) {
        Serial.println("Interfaz reclamada. Enviando datos...");
        
        usb_transfer_t *transfer;
        usb_host_transfer_alloc(64, 0, &transfer);
        memcpy(transfer->data_buffer, ticket, sizeof(ticket));
        transfer->num_bytes = sizeof(ticket);
        transfer->device_handle = dev_hdl;
        transfer->bEndpointAddress = 0x01; // Ahora que reclamamos, el 0x01 debería existir
        transfer->callback = [](usb_transfer_t *t) { Serial.println("¡IMPRESIÓN ENVIADA!"); };
        
        if (usb_host_transfer_submit(transfer) != ESP_OK) {
           // Si el 0x01 falla, intentamos el 0x02 rápido
           transfer->bEndpointAddress = 0x02;
           usb_host_transfer_submit(transfer);
        }
        proceso_completo = true;
      } else {
        Serial.printf("Error al reclamar interfaz: 0x%x\n", err);
        // Si falla la interfaz 0, el loop reintentará con la 1 en el próximo segundo
      }
    }
  } else {
    uint8_t lista[1]; int n;
    if (usb_host_device_addr_list_fill(1, lista, &n) == ESP_OK && n > 0) {
      usb_host_device_open(client_hdl, lista[0], &dev_hdl);
    }
  }
}
