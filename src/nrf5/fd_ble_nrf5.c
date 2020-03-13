#include "fd_ble.h"

#ifdef SOFTDEVICE_PRESENT

#include "fd_ble_beacon_nrf5.h"
#include "fd_log.h"

#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wold-style-declaration"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"

#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_bas.h"
#include "ble_conn_params.h"
#include "ble_dis.h"
#include "ble_gatts.h"

#include "nrf_ble_gatt.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_soc.h"

#include "app_scheduler.h"
#include "app_timer.h"

#pragma GCC diagnostic pop

#include <stdint.h>
#include <string.h>

#define FD_BLE_SERVICE_LIMIT 4
#define FD_BLE_CHARACTERISTICS_LIMIT 16
#define FD_BLE_CHANNEL_LIMIT 4

typedef struct {
    ble_gatts_char_handles_t handles;
    bool is_notification_enabled;
} fd_ble_characteristic_nrf5_t;

typedef struct {
    fd_ble_service_t *service;
    uint8_t uuid_type;
    uint16_t service_handle;
    fd_ble_characteristic_nrf5_t characteristics[FD_BLE_CHARACTERISTICS_LIMIT];
} fd_ble_service_nrf5_t;

typedef struct {
    fd_ble_l2cap_channel_t *channel;
    uint16_t local_cid;
    uint8_t stream_read_data[300];
    uint32_t stream_read_offset;
    uint32_t stream_read_length;
    uint8_t stream_write_data[300];
} fd_ble_l2cap_channel_nrf5_t;

#define APP_BLE_CONN_CFG_TAG 1

fd_ble_configuration_t *fd_ble_configuration;
fd_ble_service_nrf5_t fd_ble_services[FD_BLE_SERVICE_LIMIT];
uint32_t fd_ble_services_count;
fd_ble_l2cap_channel_nrf5_t fd_ble_channels[FD_BLE_CHANNEL_LIMIT];
uint32_t fd_ble_channel_count;
uint8_t fd_ble_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;
uint8_t fd_ble_enc_advdata[BLE_GAP_ADV_SET_DATA_SIZE_MAX];
uint8_t fd_ble_enc_scan_response_data[BLE_GAP_ADV_SET_DATA_SIZE_MAX];
ble_gap_adv_data_t fd_ble_adv_data =
{
    .adv_data =
    {
        .p_data = fd_ble_enc_advdata,
        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX
    },
    .scan_rsp_data =
    {
        .p_data = fd_ble_enc_scan_response_data,
        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX

    }
};
uint16_t fd_ble_conn_handle = BLE_CONN_HANDLE_INVALID;
uint32_t fd_ble_data_credits;

#define FD_BLE_GATT_ATT_MTU_MAX 247
//#define FD_BLE_GATT_ATT_MTU_MAX 23

const uint8_t rx_queue_size = 1;

ble_gap_sec_keyset_t ble_gap_sec_keyset;
ble_gap_conn_params_t ble_gap_conn_params;
ble_gap_evt_auth_status_t ble_gap_evt_auth_status;
ble_gap_evt_phy_update_t ble_gap_evt_phy_update;
ble_gap_data_length_params_t ble_gap_data_length_params;
ble_l2cap_evt_ch_setup_request_t ble_l2cap_evt_ch_setup_request;
ble_l2cap_evt_ch_setup_refused_t ble_l2cap_evt_ch_setup_refused;
ble_l2cap_evt_ch_setup_t ble_l2cap_evt_ch_setup;
ble_l2cap_evt_ch_rx_t ble_l2cap_evt_ch_rx;
uint32_t ble_l2cap_evt_ch_rx_sdu_len_max;
ble_data_t last_tx_sdu;
ble_l2cap_evt_ch_tx_t ble_l2cap_evt_ch_tx;
ble_l2cap_evt_ch_credit_t ble_l2cap_evt_ch_credit;
ble_gatts_evt_exchange_mtu_request_t ble_gatts_evt_exchange_mtu_request;
ble_gattc_evt_exchange_mtu_rsp_t ble_gattc_evt_exchange_mtu_rsp;

uint32_t fd_assert_fail_count = 0;

void fd_assert_fail(const char *message __attribute__((unused))) {
    ++fd_assert_fail_count;
}

#if 0
void app_error_handler(uint32_t error_code, uint32_t line_num __attribute__((unused)), const uint8_t *p_file_name __attribute__((unused))) {
    char buffer[16];
    sprintf(buffer, "app 0x%08x", (unsigned int)error_code);
    fd_assert_fail(buffer);
}
#endif

void app_error_handler_bare(uint32_t error_code) {
    char buffer[16];
    sprintf(buffer, "app 0x%08x", (unsigned int)error_code);
    fd_assert_fail(buffer);
}

void app_error_fault_handler(uint32_t FD_UNUSED id, uint32_t pc, uint32_t FD_UNUSED info) {
    char buffer[16];
    sprintf(buffer, "app 0x%08x", (unsigned int)pc);
    fd_assert_fail(buffer);
}

bool fd_ble_is_connected(void) {
    return fd_ble_conn_handle != BLE_CONN_HANDLE_INVALID;
}

uint32_t fd_ble_get_data_credits(void) {
    return fd_ble_data_credits;
}

bool fd_ble_has_data_credits(void) {
    return fd_ble_get_data_credits() > 0;
}

fd_ble_characteristic_nrf5_t *fd_ble_get_characteristic_nrf5(uint16_t uuid) {
    for (uint32_t i = 0; i < fd_ble_configuration->service_count; ++i) {
        fd_ble_service_nrf5_t *service_nrf5 = &fd_ble_services[i];
        for (uint32_t j = 0; j < service_nrf5->service->characteristics_count; ++j) {
            fd_ble_characteristic_t *characteristic = &service_nrf5->service->characteristics[j];
            if (characteristic->uuid == uuid) {
                fd_ble_characteristic_nrf5_t *characteristic_nrf5 = &service_nrf5->characteristics[j];
                return characteristic_nrf5;
            }
        }
    }
    return 0;
}

bool fd_ble_set_characteristic_value(uint16_t uuid, uint8_t *value, uint32_t length) {
    if (!fd_ble_is_connected()) {
        return false;
    }
    fd_ble_characteristic_nrf5_t *characteristic_nrf5 = fd_ble_get_characteristic_nrf5(uuid);
    ble_gatts_hvx_params_t hvx_params;
    memset(&hvx_params, 0, sizeof(hvx_params));
    hvx_params.handle = characteristic_nrf5->handles.value_handle;
    hvx_params.p_data = value;
    uint16_t len = (uint16_t)length;
    hvx_params.p_len  = &len;
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
    uint32_t err_code = sd_ble_gatts_hvx(fd_ble_conn_handle, &hvx_params);
    APP_ERROR_CHECK(err_code);
    if (fd_ble_data_credits > 0) {
        --fd_ble_data_credits;
    }
    return true;
}

void fd_ble_gap_set_device_name(void) {
    ble_gap_conn_sec_mode_t sec_mode;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
    uint8_t *name = fd_ble_configuration->name;
    uint16_t length = (uint16_t)strlen((char *)name);
    uint32_t err_code = sd_ble_gap_device_name_set(&sec_mode, name, length);
    APP_ERROR_CHECK(err_code);
}

static ble_gap_conn_params_t fd_ble_gap_conn_params(void) {
    ble_gap_conn_params_t gap_conn_params;
    memset(&gap_conn_params, 0, sizeof(gap_conn_params));
    gap_conn_params.min_conn_interval = 12; // 12 == 15 ms (1.25 ms units)
    gap_conn_params.max_conn_interval = 12; // 12 == 15 ms (1.25 ms units)
    gap_conn_params.slave_latency = 0;
    gap_conn_params.conn_sup_timeout = 400; // 4 s (10 ms units)
    return gap_conn_params;
}

void fd_ble_gap_params_initialize(void) {
    fd_ble_gap_set_device_name();

    // store the actual results here so we can verify the change. -denis
    memset(&ble_gap_conn_params, 0, sizeof(ble_gap_conn_params));

    ble_gap_conn_params_t gap_conn_params = fd_ble_gap_conn_params();
    uint32_t err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

NRF_BLE_GATT_DEF(fd_ble_gatt);

void fd_ble_gap_initialize(void) {
    ret_code_t err_code = nrf_ble_gatt_init(&fd_ble_gatt, NULL);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_ble_gatt_att_mtu_periph_set(&fd_ble_gatt, FD_BLE_GATT_ATT_MTU_MAX);
    APP_ERROR_CHECK(err_code);
}

uint32_t fd_ble_service_add_characteristic(
    fd_ble_service_nrf5_t *service_nrf5,
    fd_ble_characteristic_nrf5_t *characteristic_nrf5,
    fd_ble_characteristic_t *characteristic
) {
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;
    
    memset(&cccd_md, 0, sizeof(cccd_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);

    cccd_md.vloc = BLE_GATTS_VLOC_STACK;
    
    memset(&char_md, 0, sizeof(char_md));
    
    uint32_t flags = characteristic->flags;
    char_md.char_props.write  = (uint8_t) (flags & FD_BLE_CHARACTERISTIC_FLAG_WRITE ? 1 : 0);
    char_md.char_props.write_wo_resp = (uint8_t) (flags & FD_BLE_CHARACTERISTIC_FLAG_WRITE_WITHOUT_RESPONSE ? 1 : 0);
    char_md.char_props.notify = (uint8_t) (flags & FD_BLE_CHARACTERISTIC_FLAG_NOTIFY ? 1 : 0);
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = &cccd_md;
    char_md.p_sccd_md         = NULL;
    
    ble_uuid.type             = service_nrf5->uuid_type;
    ble_uuid.uuid             = characteristic->uuid;
    
    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    
    attr_md.vloc              = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth           = 0;
    attr_md.wr_auth           = 0;
    attr_md.vlen              = 1;
    
    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = sizeof(uint8_t);
    attr_char_value.init_offs = 0;
    attr_char_value.max_len   = 20;
    
    uint32_t err_code = sd_ble_gatts_characteristic_add(service_nrf5->service_handle, &char_md, &attr_char_value, &characteristic_nrf5->handles);
    APP_ERROR_CHECK(err_code);
    return err_code;
}

void fd_ble_service_initialize(fd_ble_service_nrf5_t *service_nrf5, fd_ble_service_t *service) {
    service_nrf5->service = service;

    ble_uuid128_t base_uuid;
    memcpy(base_uuid.uuid128, service->base_uuid, 16);
    uint32_t err_code = sd_ble_uuid_vs_add(&base_uuid, &service_nrf5->uuid_type);
    APP_ERROR_CHECK(err_code);

    ble_uuid_t ble_uuid;
    ble_uuid.type = service_nrf5->uuid_type;
    ble_uuid.uuid = service->uuid;
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &service_nrf5->service_handle);
    APP_ERROR_CHECK(err_code);

    for (uint32_t i = 0; i < service->characteristics_count; ++i) {
        fd_ble_characteristic_t *characteristic = &service->characteristics[i];
        fd_ble_characteristic_nrf5_t *characteristic_nrf5 = &service_nrf5->characteristics[i];
        fd_ble_service_add_characteristic(service_nrf5, characteristic_nrf5, characteristic);
    }
}

void fd_ble_services_initialize(fd_ble_service_t *services, uint32_t services_count) {
    fd_ble_services_count = services_count;
    for (uint32_t i = 0; i < services_count; ++i) {
        fd_ble_service_initialize(&fd_ble_services[i], &services[i]);
    }
}

void fd_ble_advertising_initialize(void) {
    ble_advdata_t advdata;
    memset(&advdata, 0, sizeof(advdata));
    advdata.name_type = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance = false;
    advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    ble_uuid_t adv_uuids[] = {{fd_ble_services[0].service->uuid, fd_ble_services[0].uuid_type}};

    advdata.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]);
    advdata.uuids_complete.p_uuids = adv_uuids;
    uint8_t encoded[32];
    uint16_t encoded_length = sizeof(encoded);
    uint32_t err_code = ble_advdata_encode(&advdata, encoded, &encoded_length);
    APP_ERROR_CHECK(err_code);
}

void fd_ble_advertising_start(void) {
    fd_ble_service_nrf5_t *service_nrf5 = &fd_ble_services[0];
    ble_uuid_t adv_uuids[] = {
        {
            .uuid = service_nrf5->service->uuid,
            .type = service_nrf5->uuid_type
        }
    };
    ble_advdata_t adv_data = {
        .name_type = BLE_ADVDATA_FULL_NAME,
        .include_appearance = false,
        .flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE,
        .uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]),
        .uuids_complete.p_uuids = adv_uuids,
    };
    uint32_t err_code = ble_advdata_encode(&adv_data, fd_ble_adv_data.adv_data.p_data, &fd_ble_adv_data.adv_data.len);
    APP_ERROR_CHECK(err_code);

    ble_gap_adv_params_t adv_params = {
        .primary_phy     = BLE_GAP_PHY_1MBPS,
        .duration        = BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED,
        .properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED,
        .p_peer_addr     = NULL,
        .filter_policy   = BLE_GAP_ADV_FP_ANY,
        .interval        = 400 // 250 ms (0.625 ms units)
    };

    err_code = sd_ble_gap_adv_set_configure(&fd_ble_adv_handle, &fd_ble_adv_data, &adv_params);
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_adv_start(fd_ble_adv_handle, APP_BLE_CONN_CFG_TAG);
    APP_ERROR_CHECK(err_code);

    fd_ble_beacon_start();
}

void fd_ble_evt_user_mem_request(ble_evt_t const FD_UNUSED *p_ble_evt) {
    sd_ble_user_mem_reply(fd_ble_conn_handle, NULL);
}

void fd_ble_evt_user_mem_release(ble_evt_t const FD_UNUSED *p_ble_evt) {
}

void fd_ble_gap_evt_connected(ble_evt_t const *p_ble_evt) {
    fd_ble_beacon_stop();

    fd_ble_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

    ble_gap_conn_params_t gap_conn_params = fd_ble_gap_conn_params();
    uint32_t err_code = sd_ble_gap_conn_param_update(fd_ble_conn_handle, &gap_conn_params);
    APP_ERROR_CHECK(err_code);

    fd_ble_data_credits = BLE_GATTS_HVN_TX_QUEUE_SIZE_DEFAULT;

    err_code = sd_ble_gatts_sys_attr_set(fd_ble_conn_handle, NULL, 0, 0);
    APP_ERROR_CHECK(err_code);
}

void fd_ble_gap_evt_disconnected(ble_evt_t const FD_UNUSED *p_ble_evt) {
    fd_ble_conn_handle = BLE_CONN_HANDLE_INVALID;
    fd_ble_data_credits = 0;

    fd_ble_advertising_start();
}

void fd_ble_gap_evt_conn_param_update(ble_evt_t const *p_ble_evt) {
    ble_gap_conn_params = p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params;
}

void fd_ble_gap_evt_sec_params_request(ble_evt_t const FD_UNUSED *p_ble_evt) {
    memset(&ble_gap_sec_keyset, 0, sizeof(ble_gap_sec_keyset));
    ble_gap_sec_params_t m_sec_params;
    m_sec_params.bond         = 1;
    m_sec_params.mitm         = 0;
    m_sec_params.io_caps      = BLE_GAP_IO_CAPS_NONE;
    m_sec_params.oob          = 0;  
    m_sec_params.min_key_size = 7;
    m_sec_params.max_key_size = 16;
    uint32_t err_code = sd_ble_gap_sec_params_reply(fd_ble_conn_handle, BLE_GAP_SEC_STATUS_SUCCESS, &m_sec_params, &ble_gap_sec_keyset);
    APP_ERROR_CHECK(err_code);
}

void fd_ble_gatts_evt_sys_attr_missing(ble_evt_t const FD_UNUSED *p_ble_evt) {
    uint32_t err_code = sd_ble_gatts_sys_attr_set(fd_ble_conn_handle, NULL, 0, 0);
    APP_ERROR_CHECK(err_code);
}

void fd_ble_gap_evt_auth_status(ble_evt_t const *p_ble_evt) {
    ble_gap_evt_auth_status = p_ble_evt->evt.gap_evt.params.auth_status;
}

void fd_ble_gap_evt_sec_info_request(ble_evt_t const FD_UNUSED *p_ble_evt) {
    uint32_t err_code = sd_ble_gap_sec_info_reply(fd_ble_conn_handle, NULL, NULL, NULL);
    APP_ERROR_CHECK(err_code);
}

void fd_ble_gatts_evt_hvn_tx_complete(ble_evt_t const *p_ble_evt) {
    uint8_t count = p_ble_evt->evt.gatts_evt.params.hvn_tx_complete.count;
    fd_ble_data_credits += count;
}

void fd_ble_gap_evt_phy_update_request(ble_evt_t const *p_ble_evt) {
    ble_gap_phys_t phys = {BLE_GAP_PHY_AUTO, BLE_GAP_PHY_AUTO};
    sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
}

void fd_ble_gap_evt_phy_update(ble_evt_t const *p_ble_evt) {
    ble_gap_evt_phy_update = p_ble_evt->evt.gap_evt.params.phy_update;
    // BLE_GAP_PHY_2MBPS == 2
}

void fd_ble_gap_evt_data_length_update_request(ble_evt_t const *p_ble_evt) {
    ble_gap_data_length_params_t params;
    // Clearing the struct will effectivly set members to BLE_GAP_DATA_LENGTH_AUTO
    memset(&params, 0, sizeof(params));
    uint32_t err_code = sd_ble_gap_data_length_update(p_ble_evt->evt.gap_evt.conn_handle, &params, 0);
    APP_ERROR_CHECK(err_code);
}

void fd_ble_gap_evt_data_length_update(ble_evt_t const *p_ble_evt) {
    ble_gap_data_length_params = p_ble_evt->evt.gap_evt.params.data_length_update.effective_params;
}

bool fd_ble_l2cap_channel_stream_is_read_available(fd_ble_l2cap_channel_t *channel) {
    return channel->read_available;
}

fd_ble_l2cap_channel_nrf5_t *fd_ble_l2cap_channel_nrf5_for_psm(uint16_t psm) {
    for (uint32_t i = 0; i < fd_ble_channel_count; ++i) {
        fd_ble_l2cap_channel_nrf5_t *channel_nrf5 = &fd_ble_channels[i];
        if (channel_nrf5->channel->protocol_service_multiplexer == psm) {
            return channel_nrf5;
        }
    }
    return 0;
}

fd_ble_l2cap_channel_nrf5_t *fd_ble_l2cap_channel_nrf5_for_local_cid(uint16_t local_cid) {
    for (uint32_t i = 0; i < fd_ble_channel_count; ++i) {
        fd_ble_l2cap_channel_nrf5_t *channel_nrf5 = &fd_ble_channels[i];
        if (channel_nrf5->local_cid == local_cid) {
            return channel_nrf5;
        }
    }
    return 0;
}

fd_ble_l2cap_channel_nrf5_t *fd_ble_l2cap_channel_nrf5_for_l2cap_channel(fd_ble_l2cap_channel_t *l2cap_channel) {
    for (uint32_t i = 0; i < fd_ble_channel_count; ++i) {
        fd_ble_l2cap_channel_nrf5_t *channel_nrf5 = &fd_ble_channels[i];
        if (channel_nrf5->channel == l2cap_channel) {
            return channel_nrf5;
        }
    }
    return 0;
}

void fd_ble_setup_l2cap_rx(fd_ble_l2cap_channel_nrf5_t *channel_nrf5) {
    channel_nrf5->stream_read_offset = 0;
    channel_nrf5->stream_read_length = 0;
    ble_data_t sdu = {
        .p_data = channel_nrf5->stream_read_data,
        .len = sizeof(channel_nrf5->stream_read_data)
    };
    uint32_t err_code = sd_ble_l2cap_ch_rx(fd_ble_conn_handle, channel_nrf5->local_cid, &sdu);
    APP_ERROR_CHECK(err_code);
}

uint32_t fd_ble_l2cap_channel_stream_read(fd_ble_l2cap_channel_t *channel, uint8_t *data, uint32_t length) {
    fd_ble_l2cap_channel_nrf5_t *channel_nrf5 = fd_ble_l2cap_channel_nrf5_for_l2cap_channel(channel);
    uint32_t n = channel_nrf5->stream_read_length;
    if (n > length) {
        n = length;
    }
    memcpy(data, &channel_nrf5->stream_read_data[channel_nrf5->stream_read_offset], n);
    channel_nrf5->stream_read_length -= n;
    if (channel_nrf5->stream_read_length == 0) {
        channel->read_available = false;
        fd_ble_setup_l2cap_rx(channel_nrf5);
    }
    return n;
}

bool fd_ble_l2cap_channel_stream_is_write_available(fd_ble_l2cap_channel_t *channel) {
    return channel->write_available;
}

uint32_t fd_ble_l2cap_channel_stream_write(fd_ble_l2cap_channel_t *channel, uint8_t *data, uint32_t length) {
    fd_ble_l2cap_channel_nrf5_t *channel_nrf5 = fd_ble_l2cap_channel_nrf5_for_l2cap_channel(channel);
    uint32_t len = sizeof(channel_nrf5->stream_write_data);
    if (len > length) {
        len = length;
    }
    ble_data_t sdu = {
        .p_data = channel_nrf5->stream_write_data,
        .len = (uint16_t)len
    };
    memcpy(channel_nrf5->stream_write_data, data, len);
    uint32_t err_code = sd_ble_l2cap_ch_tx(fd_ble_conn_handle, channel_nrf5->local_cid, &sdu);
    APP_ERROR_CHECK(err_code);
    channel->write_available = false;
    return len;
}

void fd_ble_l2cap_initialize(void) {
    ble_l2cap_evt_ch_rx_sdu_len_max = 0;
}

void fd_ble_l2cap_evt_ch_setup_request(ble_evt_t const *p_ble_evt) {
    ble_l2cap_evt_ch_setup_request = p_ble_evt->evt.l2cap_evt.params.ch_setup_request;
    fd_ble_l2cap_channel_nrf5_t *channel_nrf5 = fd_ble_l2cap_channel_nrf5_for_psm(ble_l2cap_evt_ch_setup_request.le_psm);
    // !!! reject if channel not found for that psm -denis
    channel_nrf5->local_cid = p_ble_evt->evt.l2cap_evt.local_cid;
    ble_l2cap_ch_setup_params_t ch_setup_params;
    memset(&ch_setup_params, 0, sizeof(ch_setup_params));
    ch_setup_params.le_psm = ble_l2cap_evt_ch_setup_request.le_psm;
    ch_setup_params.status = BLE_L2CAP_CH_STATUS_CODE_SUCCESS;
    ch_setup_params.rx_params.rx_mtu = BLE_L2CAP_MTU_MIN;
    ch_setup_params.rx_params.rx_mps = BLE_L2CAP_MPS_MIN;
    ch_setup_params.rx_params.sdu_buf = (ble_data_t){ .p_data = 0, .len = 0 };
    uint32_t err_code = sd_ble_l2cap_ch_setup(p_ble_evt->evt.l2cap_evt.conn_handle, &channel_nrf5->local_cid, &ch_setup_params);
    APP_ERROR_CHECK(err_code);
}

void fd_ble_l2cap_evt_ch_setup_refused(ble_evt_t const *p_ble_evt) {
    ble_l2cap_evt_ch_setup_refused = p_ble_evt->evt.l2cap_evt.params.ch_setup_refused;
}

void fd_ble_l2cap_evt_ch_setup(ble_evt_t const *p_ble_evt) {
    fd_ble_l2cap_channel_nrf5_t *channel_nrf5 = fd_ble_l2cap_channel_nrf5_for_local_cid(p_ble_evt->evt.l2cap_evt.local_cid);
    ble_l2cap_evt_ch_setup = p_ble_evt->evt.l2cap_evt.params.ch_setup;

    fd_ble_l2cap_initialize();

    fd_ble_setup_l2cap_rx(channel_nrf5);

    fd_ble_l2cap_channel_t *channel = &fd_ble_configuration->channels[0];
    channel->on_open(channel);
}

void fd_ble_l2cap_evt_ch_released(ble_evt_t const *p_ble_evt) {
    fd_ble_l2cap_channel_nrf5_t *channel_nrf5 = fd_ble_l2cap_channel_nrf5_for_local_cid(p_ble_evt->evt.l2cap_evt.local_cid);
    channel_nrf5->local_cid = 0;

    fd_ble_l2cap_channel_t *channel = &fd_ble_configuration->channels[0];
    channel->on_close(channel);
}

void fd_ble_l2cap_evt_ch_rx(ble_evt_t const *p_ble_evt) {
    fd_ble_l2cap_channel_nrf5_t *channel_nrf5 = fd_ble_l2cap_channel_nrf5_for_local_cid(p_ble_evt->evt.l2cap_evt.local_cid);
    ble_l2cap_evt_ch_rx = p_ble_evt->evt.l2cap_evt.params.rx;
    ble_data_t FD_UNUSED sdu_buf = p_ble_evt->evt.l2cap_evt.params.rx.sdu_buf;
    uint16_t len = p_ble_evt->evt.l2cap_evt.params.rx.sdu_len;
    if (len > ble_l2cap_evt_ch_rx_sdu_len_max) {
        ble_l2cap_evt_ch_rx_sdu_len_max = len;
    }

    channel_nrf5->stream_read_offset = 0;
    channel_nrf5->stream_read_length = len;

    fd_ble_l2cap_channel_t *channel = channel_nrf5->channel;
    channel->read_available = true;
    channel->on_read_available(channel);
}

void fd_ble_l2cap_evt_ch_tx(ble_evt_t const *p_ble_evt) {
    uint16_t local_cid = p_ble_evt->evt.l2cap_evt.local_cid;
    fd_ble_l2cap_channel_nrf5_t *channel_nrf5 = fd_ble_l2cap_channel_nrf5_for_local_cid(local_cid);

    ble_l2cap_evt_ch_tx = p_ble_evt->evt.l2cap_evt.params.tx;
    ble_data_t FD_UNUSED sdu_buf = p_ble_evt->evt.l2cap_evt.params.tx.sdu_buf;

    fd_ble_l2cap_channel_t *channel = channel_nrf5->channel;
    channel->write_available = true;
    channel->on_write_available(channel);
}

void fd_ble_l2cap_evt_ch_credit(ble_evt_t const *p_ble_evt) {
    ble_l2cap_evt_ch_credit = p_ble_evt->evt.l2cap_evt.params.credit;
    // send more data...
}

// tx or rx buffer released (typically when channel is closed)
void fd_ble_l2cap_evt_ch_sdu_buf_released(ble_evt_t const *p_ble_evt) {
    ble_data_t FD_UNUSED sdu_buf = p_ble_evt->evt.l2cap_evt.params.ch_sdu_buf_released.sdu_buf;
}

void fd_ble_gatts_evt_exchange_mtu_request(ble_evt_t const *p_ble_evt) {
    ble_gatts_evt_exchange_mtu_request = p_ble_evt->evt.gatts_evt.params.exchange_mtu_request;
    uint16_t FD_UNUSED conn_handle = p_ble_evt->evt.gatts_evt.conn_handle;
    uint16_t client_rx_mtu = p_ble_evt->evt.gatts_evt.params.exchange_mtu_request.client_rx_mtu;
    if (client_rx_mtu > FD_BLE_GATT_ATT_MTU_MAX) {
        client_rx_mtu = FD_BLE_GATT_ATT_MTU_MAX;
    }
// !!! this causes an exception - why? -denis
//    uint32_t err_code = sd_ble_gatts_exchange_mtu_reply(conn_handle, client_rx_mtu);
//    APP_ERROR_CHECK(err_code);
}

void fd_ble_gattc_evt_exchange_mtu_rsp(ble_evt_t const *p_ble_evt) {
    ble_gattc_evt_exchange_mtu_rsp = p_ble_evt->evt.gattc_evt.params.exchange_mtu_rsp;
}

void fd_ble_gatts_evt_write(ble_evt_t const *p_ble_evt) {
    const ble_gatts_evt_write_t *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
    for (uint32_t i = 0; i < fd_ble_services_count; ++i) {
        fd_ble_service_nrf5_t *service_nrf5 = &fd_ble_services[i];
        for (uint32_t j = 0; j < service_nrf5->service->characteristics_count; ++j) {
            fd_ble_characteristic_nrf5_t *characteristic_nrf5 = &service_nrf5->characteristics[j];
            if ((p_evt_write->handle == characteristic_nrf5->handles.cccd_handle) && (p_evt_write->len == 2)) {
                characteristic_nrf5->is_notification_enabled = ble_srv_is_notification_enabled(p_evt_write->data);
                return;
            } else
            if (p_evt_write->handle == characteristic_nrf5->handles.value_handle) {
                fd_ble_characteristic_t *characteristic = &service_nrf5->service->characteristics[j];
                if (characteristic->value_handler) {
                    characteristic->value_handler(characteristic->uuid, p_evt_write->data, p_evt_write->len);
                }
                return;
            }
        }
    }
}

void fd_ble_on_ble_evt(ble_evt_t const *p_ble_evt) {
    switch (p_ble_evt->header.evt_id) {
        case BLE_EVT_USER_MEM_REQUEST: {
            fd_ble_evt_user_mem_request(p_ble_evt);
        } break;
        case BLE_EVT_USER_MEM_RELEASE: {
            fd_ble_evt_user_mem_release(p_ble_evt);
        } break;
        case BLE_GAP_EVT_CONNECTED: {
            fd_ble_gap_evt_connected(p_ble_evt);
        } break;
        case BLE_GAP_EVT_DISCONNECTED: {
            fd_ble_gap_evt_disconnected(p_ble_evt);
        } break;
        case BLE_GAP_EVT_CONN_PARAM_UPDATE: {
            fd_ble_gap_evt_conn_param_update(p_ble_evt);
        } break;
        case BLE_GAP_EVT_SEC_PARAMS_REQUEST: {
            fd_ble_gap_evt_sec_params_request(p_ble_evt);
        } break;
        case BLE_GATTS_EVT_SYS_ATTR_MISSING: {
            fd_ble_gatts_evt_sys_attr_missing(p_ble_evt);
        } break;
        case BLE_GAP_EVT_AUTH_STATUS: {
            fd_ble_gap_evt_auth_status(p_ble_evt);
        } break;
        case BLE_GAP_EVT_SEC_INFO_REQUEST: {
            fd_ble_gap_evt_sec_info_request(p_ble_evt);
        } break;
        case BLE_GATTS_EVT_HVN_TX_COMPLETE: {
            fd_ble_gatts_evt_hvn_tx_complete(p_ble_evt);
        } break;
        case BLE_GATTS_EVT_WRITE: {
            fd_ble_gatts_evt_write(p_ble_evt);
        } break;
        case BLE_GAP_EVT_PHY_UPDATE_REQUEST: {
            fd_ble_gap_evt_phy_update_request(p_ble_evt);
        } break;
        case BLE_GAP_EVT_PHY_UPDATE: {
            fd_ble_gap_evt_phy_update(p_ble_evt);
        } break;
        case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST: {
            fd_ble_gap_evt_data_length_update_request(p_ble_evt);
        } break;
        case BLE_GAP_EVT_DATA_LENGTH_UPDATE: {
            fd_ble_gap_evt_data_length_update(p_ble_evt);
        } break;
        case BLE_L2CAP_EVT_CH_SETUP_REQUEST: {
            fd_ble_l2cap_evt_ch_setup_request(p_ble_evt);
        } break;
        case BLE_L2CAP_EVT_CH_SETUP_REFUSED: {
            fd_ble_l2cap_evt_ch_setup_refused(p_ble_evt);
        } break;
        case BLE_L2CAP_EVT_CH_SETUP: {
            fd_ble_l2cap_evt_ch_setup(p_ble_evt);
        } break;
        case BLE_L2CAP_EVT_CH_RELEASED: {
            fd_ble_l2cap_evt_ch_released(p_ble_evt);
        } break;
        case BLE_L2CAP_EVT_CH_CREDIT: {
            fd_ble_l2cap_evt_ch_credit(p_ble_evt);
        } break;
        case BLE_L2CAP_EVT_CH_RX: {
            fd_ble_l2cap_evt_ch_rx(p_ble_evt);
        } break;
        case BLE_L2CAP_EVT_CH_TX: {
            fd_ble_l2cap_evt_ch_tx(p_ble_evt);
        } break;
        case BLE_L2CAP_EVT_CH_SDU_BUF_RELEASED: {
            fd_ble_l2cap_evt_ch_sdu_buf_released(p_ble_evt);
        } break;
        case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST: {
            fd_ble_gatts_evt_exchange_mtu_request(p_ble_evt);
        } break;
        case BLE_GATTC_EVT_EXCHANGE_MTU_RSP: {
            fd_ble_gattc_evt_exchange_mtu_rsp(p_ble_evt);
        } break;
        default:
            break;
    }
}

void fd_ble_evt_dispatch(ble_evt_t const *p_ble_evt, void FD_UNUSED *p_context) {
    fd_ble_on_ble_evt(p_ble_evt);
}

void fd_ble_soc_evt_handler(uint32_t event_id, void *p_context);

// Register a handler for BLE events.
#define APP_BLE_OBSERVER_PRIO 1
NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, fd_ble_evt_dispatch, 0);

// Register handlers for SoC events.
#define APP_SOC_OBSERVER_PRIO 1
NRF_SDH_SOC_OBSERVER(m_soc_observer, APP_SOC_OBSERVER_PRIO, fd_ble_soc_evt_handler, NULL);

void fd_ble_stack_initialize(void) {
    uint32_t err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Configure L2CAP
    ble_cfg_t ble_cfg;
    memset(&ble_cfg, 0, sizeof(ble_cfg));
    ble_cfg.conn_cfg.conn_cfg_tag = APP_BLE_CONN_CFG_TAG;
    ble_cfg.conn_cfg.params.l2cap_conn_cfg.rx_mps        = 512; // BLE_L2CAP_MPS_MIN;
    ble_cfg.conn_cfg.params.l2cap_conn_cfg.rx_queue_size = rx_queue_size;
    ble_cfg.conn_cfg.params.l2cap_conn_cfg.tx_mps        = 512; // BLE_L2CAP_MPS_MIN;
    ble_cfg.conn_cfg.params.l2cap_conn_cfg.tx_queue_size = 10;
    ble_cfg.conn_cfg.params.l2cap_conn_cfg.ch_count      = 1;
    err_code = sd_ble_cfg_set(BLE_CONN_CFG_L2CAP, &ble_cfg, ram_start);
    APP_ERROR_CHECK(err_code);

    // Configure MTU MAX
    memset(&ble_cfg, 0, sizeof(ble_cfg));
    ble_cfg.conn_cfg.conn_cfg_tag = APP_BLE_CONN_CFG_TAG;
    ble_cfg.conn_cfg.params.gatt_conn_cfg.att_mtu = FD_BLE_GATT_ATT_MTU_MAX;
    err_code = sd_ble_cfg_set(BLE_CONN_CFG_GATT, &ble_cfg, ram_start);
    APP_ERROR_CHECK(err_code);

    // Configure the maximum event length
    memset(&ble_cfg, 0x00, sizeof(ble_cfg));
    ble_cfg.conn_cfg.conn_cfg_tag = APP_BLE_CONN_CFG_TAG;
    ble_cfg.conn_cfg.params.gap_conn_cfg.event_length = 320;
    ble_cfg.conn_cfg.params.gap_conn_cfg.conn_count = BLE_GAP_CONN_COUNT_DEFAULT;
    err_code = sd_ble_cfg_set(BLE_CONN_CFG_GAP, &ble_cfg, ram_start);
    APP_ERROR_CHECK(err_code);

    // Configure service changed
    memset(&ble_cfg, 0, sizeof(ble_cfg));
    ble_cfg.gatts_cfg.service_changed.service_changed = 1;
    err_code = sd_ble_cfg_set(BLE_GATTS_CFG_SERVICE_CHANGED, &ble_cfg, ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);
}

void fd_ble_disconnect(void) {
    if (fd_ble_conn_handle != BLE_CONN_HANDLE_INVALID) {
        sd_ble_gap_disconnect(fd_ble_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    }
}

typedef enum {
    flash_operation_result_none,
    flash_operation_result_pending,
    flash_operation_result_success,
    flash_operation_result_error,
} flash_operation_result_t;

volatile flash_operation_result_t fd_ble_flash_operation_result;

flash_operation_result_t fd_ble_flash_wait(void) {
    while (fd_ble_flash_operation_result == flash_operation_result_pending) {
    }
    flash_operation_result_t result = fd_ble_flash_operation_result;
    fd_ble_flash_operation_result = flash_operation_result_none;
    return result;
}

void fd_ble_flash_page_erase(uint32_t address) {
    do {
        fd_ble_flash_operation_result = flash_operation_result_pending;
        uint32_t page = address / 4096;
        uint32_t err_code = sd_flash_page_erase(page);
        APP_ERROR_CHECK(err_code);
        if (err_code != NRF_SUCCESS) {
            return;
        }
    } while (fd_ble_flash_wait() != flash_operation_result_success);
}

void fd_ble_flash_write(uint32_t address, const void *data, size_t length) {
    do {
        fd_ble_flash_operation_result = flash_operation_result_pending;
        uint32_t words = length / 4;
        uint32_t err_code = sd_flash_write((uint32_t *)address, data, words);
        APP_ERROR_CHECK(err_code);
        if (err_code != NRF_SUCCESS) {
            return;
        }
    } while (fd_ble_flash_wait() != flash_operation_result_success);
}

void fd_ble_soc_evt_handler(uint32_t event_id, void FD_UNUSED *p_context) {
    fd_ble_beacon_on_sys_evt(event_id);
    switch (event_id) {
        case NRF_EVT_FLASH_OPERATION_SUCCESS:
            fd_ble_flash_operation_result = flash_operation_result_success;
            break;
        case NRF_EVT_FLASH_OPERATION_ERROR:
            fd_ble_flash_operation_result = flash_operation_result_error;
            break;
        default:
            break;
    }
}

APP_TIMER_DEF(fd_ble_timer_id);

void fd_ble_timer_handler(void *context FD_UNUSED) {
    if (fd_ble_configuration->on_tick) {
        fd_ble_configuration->on_tick();
    }
}

static
void fd_ble_beacon_error_handler(uint32_t nrf_error) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "beacon: %d", nrf_error);
    fd_log(buffer);
}

#define APP_COMPANY_IDENTIFIER               0x0059                                     /**< Company identifier for Nordic Semiconductor ASA. as per www.bluetooth.org. */

#define BEACON_UUID 0xff, 0xfe, 0x2d, 0x12, 0x1e, 0x4b, 0x0f, 0xa4,\
                    0x99, 0x4e, 0xce, 0xb5, 0x31, 0xf4, 0x05, 0x45
#define BEACON_ADV_INTERVAL                  400                                        /**< The Beacon's advertising interval, in milliseconds*/
#define BEACON_MAJOR                         0x1234                                     /**< The Beacon's Major*/
#define BEACON_MINOR                         0x5678                                     /**< The Beacon's Minor*/
#define BEACON_RSSI                          0xC3                                       /**< The Beacon's measured RSSI at 1 meter distance in dBm. */

void fd_ble_initialize_beacon(void) {
    fd_ble_beacon_init_t beacon_init;

    static uint8_t beacon_uuid[] = {BEACON_UUID};

    memcpy(beacon_init.uuid.uuid128, beacon_uuid, sizeof(beacon_uuid));
    beacon_init.adv_interval  = BEACON_ADV_INTERVAL;
    beacon_init.major         = BEACON_MAJOR;
    beacon_init.minor         = BEACON_MINOR;
    beacon_init.manuf_id      = APP_COMPANY_IDENTIFIER;
    beacon_init.rssi          = BEACON_RSSI;
    beacon_init.error_handler = fd_ble_beacon_error_handler;

    uint32_t err_code = sd_ble_gap_addr_get(&beacon_init.beacon_addr);
    APP_ERROR_CHECK(err_code);
    // Increment device address by 2 for beacon advertising.
    beacon_init.beacon_addr.addr[0] += 2;

    fd_ble_beacon_init(&beacon_init);
}

void fd_ble_initialize(fd_ble_configuration_t *configuration) {
    fd_ble_configuration = configuration;

    for (uint32_t i = 0; i < configuration->channel_count; ++i) {
        fd_ble_l2cap_channel_t *channel = &configuration->channels[i];
        channel->read_available = false;
        channel->is_read_available = fd_ble_l2cap_channel_stream_is_read_available;
        channel->read = fd_ble_l2cap_channel_stream_read;
        channel->write_available = false;
        channel->is_write_available = fd_ble_l2cap_channel_stream_is_write_available;
        channel->write = fd_ble_l2cap_channel_stream_write;

        fd_ble_l2cap_channel_nrf5_t *channel_nrf5 = &fd_ble_channels[i];
        channel_nrf5->channel = channel;
        channel_nrf5->local_cid = 0;
    }
    fd_ble_channel_count = configuration->channel_count;

    fd_ble_flash_operation_result = flash_operation_result_none;

    APP_SCHED_INIT(32, 32);

    fd_ble_stack_initialize();
    fd_ble_gap_params_initialize();
    fd_ble_gap_initialize();
    fd_ble_l2cap_initialize();
    fd_ble_services_initialize(configuration->services, configuration->service_count);
    fd_ble_advertising_initialize();
    fd_ble_initialize_beacon();
    fd_ble_advertising_start();

    uint32_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_create(&fd_ble_timer_id, APP_TIMER_MODE_REPEATED, fd_ble_timer_handler);
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_start(fd_ble_timer_id, APP_TIMER_TICKS(100), 0);
    APP_ERROR_CHECK(err_code);
}

void fd_ble_main_loop(void) {
    while (true) {
        app_sched_execute();
        uint32_t result = sd_app_evt_wait();
        APP_ERROR_CHECK(result);
    }
}

#endif
