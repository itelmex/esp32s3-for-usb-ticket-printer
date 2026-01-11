#include "usb/usb_host.h"

// --- DATOS A IMPRIMIR ---
uint8_t ticket[] = {
  0x1B, 0x40,                         // Inicializar
  0x1B, 0x61, 0x01,                   // Centrado
  'T', 'E', 'S', 'T', ' ', 'C', 'O', 'N', 'E', 'X', 'I', 'O', 'N', '\n',
  'P', 'A', 'S', 'O', ' ', 'A', ' ', 'P', 'A', 'S', 'O', '\n',
  0x0A, 0x0A, 0x0A, 0x0A              // Avance
};

// --- GLOBALES ---
usb_host_client_handle_t client_hdl;
usb_device_handle_t dev_hdl = NULL;
uint8_t out_endpoint = 0x00;
bool proceso_finalizado = false;

// --- CALLBACK DE TRANSFERENCIA ---
void transfer_cb(usb_transfer_t *transfer) {
  Serial.println(">> ¡Transferencia completada físicamente!");
  usb_host_transfer_free(transfer);
}

// --- ANALIZADOR DE DESCRIPTORES ---
void buscar_endpoint_impresora(usb_device_handle_t handle) {
  const usb_config_desc_t *config_desc;
  if (usb_host_get_active_config_descriptor(handle, &config_desc) != ESP_OK) return;

  int offset = 0;
  const usb_standard_desc_t *next_desc = (const usb_standard_desc_t *)config_desc;

  while (next_desc != NULL) {
    if (next_desc->bDescriptorType == 0x05) { // 0x05 es Endpoint Descriptor
      const usb_ep_desc_t *ep = (const usb_ep_desc_t *)next_desc;
      
      // Bulk = 2, Bit 7 de bEndpointAddress (0 = OUT, 1 = IN)
      if ((ep->bmAttributes & 0x03) == 0x02 && !(ep->bEndpointAddress & 0x80)) {
        out_endpoint = ep->bEndpointAddress;
        Serial.printf(">> Endpoint Hallado: 0x%02X\n", out_endpoint);
        return;
      }
    }
    next_desc = usb_parse_next_descriptor(next_desc, config_desc->wTotalLength, &offset);
  }
}

// --- CLIENTE DE EVENTOS ---
void client_event_cb(const usb_host_client_event_msg_t *event, void *arg) {
  if (event->event == USB_HOST_CLIENT_EVENT_NEW_DEV) {
    Serial.printf(">> Evento: Nuevo dispositivo detectado (Dir: %d)\n", event->new_dev.address);
    usb_host_device_open(client_hdl, event->new_dev.address, &dev_hdl);
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n--- INICIANDO ESCANEO USB ---");

  // 1. Instalar Host
  usb_host_config_t host_config = { .intr_flags = ESP_INTR_FLAG_LEVEL1 };
  usb_host_install(&host_config);

  // 2. Registrar Cliente
  usb_host_client_config_t client_config = {
    .is_synchronous = true,
    .max_num_event_msg = 5,
    .async = { .client_event_callback = client_event_cb, .callback_arg = NULL }
  };
  usb_host_client_register(&client_config, &client_hdl);
  
  Serial.println("Esperando conexión de impresora...");
}

void loop() {
  uint32_t event_flags;
  // Manejamos eventos del sistema
  usb_host_lib_handle_events(1, &event_flags);

  // LÓGICA DE DETECCIÓN FORZADA (Si el evento NEW_DEV falló)
  if (dev_hdl == NULL) {
    uint8_t addr_list[5];
    int num_devs;
    if (usb_host_device_addr_list_fill(5, addr_list, &num_devs) == ESP_OK && num_devs > 0) {
      Serial.printf(">> Detectado dispositivo en dirección %d sin abrir. Abriendo...\n", addr_list[0]);
      usb_host_device_open(client_hdl, addr_list[0], &dev_hdl);
    }
  }

  // LÓGICA DE IMPRESIÓN
  if (dev_hdl != NULL && !proceso_finalizado) {
    if (out_endpoint == 0) {
      buscar_endpoint_impresora(dev_hdl);
    }

    if (out_endpoint != 0) {
      // Intentamos reclamar la interfaz 0 (la más común)
      esp_err_t err = usb_host_interface_claim(client_hdl, dev_hdl, 0, 0);
      if (err == ESP_OK) {
        Serial.println(">> Interfaz reclamada. Enviando datos...");
        
        usb_transfer_t *transfer;
        usb_host_transfer_alloc(sizeof(ticket), 0, &transfer);
        memcpy(transfer->data_buffer, ticket, sizeof(ticket));
        transfer->num_bytes = sizeof(ticket);
        transfer->device_handle = dev_hdl;
        transfer->bEndpointAddress = out_endpoint;
        transfer->callback = transfer_cb;

        if (usb_host_transfer_submit(transfer) == ESP_OK) {
          proceso_finalizado = true;
        } else {
          usb_host_transfer_free(transfer);
        }
      } else {
        Serial.printf(">> Error reclamando interfaz: 0x%x\n", err);
        delay(1000); // Evitar spam de errores
      }
    }
  }
}
