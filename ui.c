#include "main.h"

#include "ui_edits.h"
#include "ui_buttons.h"
#include "ui_dropdown.h"

// Application-wide language setting
UI_LANG_ID LANG;

/***** MAYBE_I18NAL_STRING helpers start *****/

void maybe_i18nal_string_set_plain(MAYBE_I18NAL_STRING *mis, char_t *str, STRING_IDX length) {
    mis->plain.str = str;
    mis->plain.length = length;
    mis->i18nal = UI_STRING_ID_INVALID;
}

void maybe_i18nal_string_set_i18nal(MAYBE_I18NAL_STRING *mis, UI_STRING_ID string_id) {
    mis->plain.str = NULL;
    mis->plain.length = 0;
    mis->i18nal = string_id;
}

STRING* maybe_i18nal_string_get(MAYBE_I18NAL_STRING *mis) {
    if(mis->plain.str) {
        return &mis->plain;
    } else {
        return SPTRFORLANG(LANG, mis->i18nal);
    }
}

_Bool maybe_i18nal_string_is_valid(MAYBE_I18NAL_STRING *mis) {
    return (mis->plain.str || ((UI_STRING_ID_INVALID != mis->i18nal) && (mis->i18nal <= STRS_MAX)));
}

/***** MAYBE_I18NAL_STRING helpers end *****/

void draw_avatar_image(UTOX_NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t targetwidth, uint32_t targetheight)
{
    /* get smallest of width or height */
    double scale = (width > height) ?
                      (double)targetheight / height :
                      (double)targetwidth / width;

    image_set_scale(image, scale);
    image_set_filter(image, FILTER_BILINEAR);

    /* set position to show the middle of the image in the center  */
    int xpos = (int) ((double)width * scale / 2 - (double)targetwidth / 2);
    int ypos = (int) ((double)height * scale / 2 - (double)targetheight / 2);

    draw_image(image, x, y, targetwidth, targetheight, xpos, ypos);

    image_set_scale(image, 1.0);
    image_set_filter(image, FILTER_NEAREST);
}

/* Top left self interface Avatar, name, statusmsg, status icon */
static void drawself(void)
{
    setcolor(!button_name.mouseover ? COLOR_MENU_TEXT : COLOR_MENU_SUBTEXT);
    setfont(FONT_SELF_NAME);
    drawtextrange(SELF_NAME_X, SELF_STATUS_X, SELF_NAME_Y, self.name, self.name_length);

    // @TODO: separate these colors if needed (COLOR_MAIN_HINTTEXT)
    setcolor(!button_statusmsg.mouseover ? COLOR_MENU_SUBTEXT : COLOR_MAIN_HINTTEXT);
    setfont(FONT_STATUS);
    drawtextrange(SELF_MSG_X, SELF_STATUS_X, SELF_MSG_Y, self.statusmsg, self.statusmsg_length);

    // draw avatar or default image
    if (self_has_avatar()) {
        draw_avatar_image(self.avatar.image, SELF_AVATAR_X, SELF_AVATAR_Y, self.avatar.width, self.avatar.height, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH);
    } else {
        drawalpha(BM_CONTACT, SELF_AVATAR_X, SELF_AVATAR_Y, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH, COLOR_MENU_TEXT);
    }

    drawalpha(BM_STATUSAREA, SELF_STATUS_X, SELF_STATUS_Y, BM_STATUSAREA_WIDTH, BM_STATUSAREA_HEIGHT, button_status.mouseover ? COLOR_LIST_HOVER_BACKGROUND : COLOR_LIST_BACKGROUND);

    uint8_t status = tox_connected ? self.status : 3;
    drawalpha(BM_ONLINE + status, SELF_STATUS_X + BM_STATUSAREA_WIDTH / 2 - BM_STATUS_WIDTH / 2, SELF_STATUS_Y + BM_STATUSAREA_HEIGHT / 2 - BM_STATUS_WIDTH / 2, BM_STATUS_WIDTH, BM_STATUS_WIDTH, status_color[status]);
}

/* Header for friend chat window */
static void drawfriend(int x, int y, int w, int height)
{
    FRIEND *f = sitem->data;

    // draw avatar or default image
    if (friend_has_avatar(f)) {
        draw_avatar_image(f->avatar.image, LIST_RIGHT + SCALE * 5, SCALE * 5, f->avatar.width, f->avatar.height, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH);
    } else {
        drawalpha(BM_CONTACT, LIST_RIGHT + SCALE * 5, SCALE * 5, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH, COLOR_MAIN_TEXT);
    }

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_TITLE);
    drawtextrange(LIST_RIGHT + 30 * SCALE, utox_window_width - 92 * SCALE, 9 * SCALE, f->name, f->name_length);

    setcolor(COLOR_MAIN_SUBTEXT);
    setfont(FONT_STATUS);
    drawtextrange(LIST_RIGHT + 30 * SCALE, utox_window_width - 92 * SCALE, 16 * SCALE, f->status_message, f->status_length);

    if (f->typing) {
        int typing_y = ((y + height) + MESSAGES_BOTTOM);
        setfont(FONT_MISC);
        // @TODO: separate these colors if needed
        setcolor(COLOR_MAIN_HINTTEXT);
        drawtextwidth_right(x, MESSAGES_X - NAME_OFFSET, typing_y, f->name, f->name_length);
        drawtextwidth(x + MESSAGES_X, x + w, typing_y, S(IS_TYPING), SLEN(IS_TYPING));
    }
}

static void drawgroup(int UNUSED(x), int UNUSED(y), int UNUSED(w), int UNUSED(height))
{
    GROUPCHAT *g = sitem->data;

    drawalpha(BM_GROUP, LIST_RIGHT + SCALE * 5, SCALE * 5, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH, COLOR_MAIN_TEXT);

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_TITLE);
    drawtextrange(LIST_RIGHT + 30 * SCALE, utox_window_width - 32 * SCALE, 1 * SCALE, g->name, g->name_length);

    setcolor(COLOR_MAIN_SUBTEXT);
    setfont(FONT_STATUS);
    drawtextrange(LIST_RIGHT + 30 * SCALE, utox_window_width - 32 * SCALE, 8 * SCALE, g->topic, g->topic_length);

    uint32_t i = 0;
    int k = LIST_RIGHT + 30 * SCALE;

    uint64_t time = get_time();

    unsigned int pos_y = 15;
    while(i < g->peers)
    {
        uint8_t *name = g->peername[i];
        if(name)
        {
            uint8_t buf[134];
            memcpy(buf, name + 1, name[0]);
            memcpy(buf + name[0], ", ", 2);

            int w = textwidth(buf, name[0] + 2);
            if (i == g->our_peer_number) {
                setcolor(COLOR_GROUP_SELF);
            } else if (time - g->last_recv_audio[i] <= (uint64_t)1 * 1000 * 1000 * 1000) {
                setcolor(COLOR_GROUP_AUDIO);
            } else {
                setcolor(COLOR_GROUP_PEER);
            }

            if(k + w >= (utox_window_width - 32 * SCALE)) {
                if (pos_y == 15) {
                    pos_y += 6;
                    k = LIST_RIGHT + 30 * SCALE;
                } else {
                    drawtext(k, pos_y * SCALE, (uint8_t*)"...", 3);
                    break;
                }
            }

            drawtext(k, pos_y * SCALE, buf, name[0] + 2);

            k += w;
        }
        i++;
    }
}

/* Draw an invite to be a friend window */
static void drawfriendreq(int UNUSED(x), int UNUSED(y), int UNUSED(w), int UNUSED(height))
{
    FRIENDREQ *req = sitem->data;

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(LIST_RIGHT + SCALE * 5, SCALE * 10, FRIENDREQUEST);

    setcolor(COLOR_MAIN_SUBTEXT);
    setfont(FONT_STATUS);
    drawtextrange(LIST_RIGHT + 5 * SCALE, utox_window_width, 20 * SCALE, req->msg, req->length);
}

/* Draw add a friend window */
static void drawadd(int UNUSED(x), int UNUSED(y), int UNUSED(w), int height)
{
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(LIST_RIGHT + SCALE * 5, SCALE * 10, ADDFRIENDS);

    setcolor(COLOR_MAIN_SUBTEXT);
    setfont(FONT_TEXT);
    drawstr(LIST_RIGHT + SCALE * 5, LIST_Y + SCALE * 5, TOXID);

    drawstr(LIST_RIGHT + SCALE * 5, LIST_Y + SCALE * 29, MESSAGE);

    if(addfriend_status) {
        setfont(FONT_MISC);
        setcolor(C_RED);

        STRING *str;
        switch(addfriend_status) {
        case ADDF_SENT:
            str = SPTR(REQ_SENT); break;
        case ADDF_DISCOVER:
            str = SPTR(REQ_RESOLVE); break;
        case ADDF_BADNAME:
            str = SPTR(REQ_INVALID_ID); break;
        case ADDF_NONAME:
            str = SPTR(REQ_EMPTY_ID); break;
        case ADDF_TOOLONG: //if message length is too long.
            str = SPTR(REQ_LONG_MSG); break;
        case ADDF_NOMESSAGE: //if no message (message length must be >= 1 byte).
            str = SPTR(REQ_NO_MSG); break;
        case ADDF_OWNKEY: //if user's own key.
            str = SPTR(REQ_SELF_ID); break;
        case ADDF_ALREADYSENT: //if friend request already sent or already a friend.
            str = SPTR(REQ_ALREADY_FRIENDS); break;
        case ADDF_BADCHECKSUM: //if bad checksum in address.
            str = SPTR(REQ_BAD_CHECKSUM); break;
        case ADDF_SETNEWNOSPAM: //if the friend was already there but the nospam was different.
            str = SPTR(REQ_BAD_NOSPAM); break;
        case ADDF_NOMEM: //if increasing the friend list size fails.
            str = SPTR(REQ_NO_MEMORY); break;
        case ADDF_UNKNOWN: //for unknown error.
        case ADDF_NONE: //this case must never be rendered, but if it does, assume it's an error
        default:
            str = SPTR(REQ_UNKNOWN); break;
        }

        drawtextmultiline(LIST_RIGHT + SCALE * 5, utox_window_width - BM_SBUTTON_WIDTH - 5 * SCALE, LIST_Y + SCALE * 83, 0, height, font_small_lineheight, str->str, str->length, 0xFFFF, 0, 0, 0, 1);
    }
}

/* Top bar for user settings */
static void drawsettings_header(int UNUSED(x), int UNUSED(y), int UNUSED(width), int UNUSED(height)){
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(LIST_RIGHT + SCALE * 5, SCALE * 5, UTOX_SETTINGS);
    #ifdef GIT_VERSION
        setfont(FONT_TEXT);
        drawtext(LIST_RIGHT + SCALE * 60, SCALE * 5, (uint8_t*)GIT_VERSION, strlen(GIT_VERSION));
    #endif
}

/* draw switch profile top bar */
/* Current TODO */
static void drawtransfer(int UNUSED(x), int UNUSED(y), int UNUSED(width), int UNUSED(height)){
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(LIST_RIGHT + SCALE * 5, SCALE * 10, SWITCHPROFILE);
}

/* Text content for settings page */
static void drawsettings_text_utox(int x, int y, int w, int h){
    setcolor(COLOR_MAIN_TEXT);
    drawstr(LIST_RIGHT + SCALE * 5, y + 5   * SCALE, NAME);
    drawstr(LIST_RIGHT + SCALE * 5, y + 30  * SCALE, STATUSMESSAGE);
    setfont(FONT_SELF_NAME);
    drawstr(LIST_RIGHT + SCALE * 5, y + 55  * SCALE, TOXID);
    setfont(FONT_TEXT);
    drawstr(LIST_RIGHT + SCALE * 5, y + 75  * SCALE, LANGUAGE);
}

static void drawsettings_text_network(int x, int y, int w, int UNUSED(height)){
    setfont(FONT_MISC);
    setcolor(C_RED);
    drawstr(LIST_RIGHT  + 5   * SCALE, y + 5 * SCALE, WARNING);

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_TEXT);
    drawstr(LIST_RIGHT  + 5   * SCALE, y + 15 * SCALE, IPV6);
    drawstr(LIST_RIGHT  + 55  * SCALE, y + 15 * SCALE, UDP);
    drawstr(LIST_RIGHT  + 5   * SCALE, y + 30 * SCALE, PROXY);
    setfont(FONT_SELF_NAME);
    drawtext(LIST_RIGHT + 132 * SCALE, y + 42 * SCALE, (uint8_t*)":", 1);
}

static void drawsettings_text_ui(int x, int y, int w, int UNUSED(height)){
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_TEXT);
    drawstr(LIST_RIGHT + 75 * SCALE, y + 5 * SCALE, DPI);
    drawstr(LIST_RIGHT + 5  * SCALE, y + 5 * SCALE, THEME);
    drawstr(LIST_RIGHT + 5  * SCALE, y + 30 * SCALE, LOGGING);
    drawstr(LIST_RIGHT + 5  * SCALE, y + 55 * SCALE, CLOSE_TO_TRAY);
    drawstr(LIST_RIGHT + 75 * SCALE, y + 55 * SCALE, START_IN_TRAY);
    drawstr(LIST_RIGHT + 5  * SCALE, y + 80 * SCALE, AUTO_STARTUP);
    drawstr(LIST_RIGHT + 5  * SCALE, y + 105 * SCALE, SEND_TYPING_NOTIFICATIONS);
}

static void drawsettings_text_av(int x, int y, int w, int UNUSED(height)){
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_TEXT);
    drawstr(LIST_RIGHT + SCALE * 5,   y + SCALE * 5,  RINGTONE);
    drawstr(LIST_RIGHT + SCALE * 50,   y + SCALE * 5,  AUDIONOTIFICATIONS_MESSAGES);
    #ifdef AUDIO_FILTERING
    drawstr(LIST_RIGHT + SCALE * 100, y + SCALE * 5,  AUDIOFILTERING);
    #endif
    drawstr(LIST_RIGHT + SCALE * 5,   y + SCALE * 35, AUDIOINPUTDEVICE);
    drawstr(LIST_RIGHT + SCALE * 5,   y + SCALE * 60, AUDIOOUTPUTDEVICE);
    drawstr(LIST_RIGHT + SCALE * 5,   y + SCALE * 85, VIDEOINPUTDEVICE);
    setfont(FONT_SELF_NAME);
    drawstr(LIST_RIGHT + SCALE * 5,   y + SCALE * 115, PREVIEW);
}

static void drawsettings_sub_header(int x, int y, int w, int UNUSED(height)){
    setfont(FONT_SELF_NAME);

    /* Draw the text and bars for general settings */
    setcolor(!button_settings_sub_utox.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_SUBTEXT);
    drawstr(   x + 5   * SCALE, y + 5 * SCALE, PROFILE);
    if (panel_settings_utox.disabled) {
        drawhline( x + 0   * SCALE, y + 15 * SCALE, x + 65 * SCALE, COLOR_EDGE_NORMAL);
    } else {
        drawhline( x + 0   * SCALE, y + 0, x + 65 * SCALE, COLOR_EDGE_ACTIVE);
        drawhline( x + 0   * SCALE, y + 1, x + 65 * SCALE, COLOR_EDGE_ACTIVE);
    }
    drawvline( x + 65  * SCALE, y + 0 * SCALE, y + 15 * SCALE, COLOR_EDGE_NORMAL);

    /* Draw the text and bars for network settings */
    setcolor(!button_settings_sub_net.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_SUBTEXT);
    drawstr(   x + 70  * SCALE, y + 5 * SCALE, NETWORK);
    if (panel_settings_net.disabled) {
        drawhline( x + 65  * SCALE, y + 15 * SCALE, x + 110 * SCALE, COLOR_EDGE_NORMAL);
    } else {
        drawhline( x + 65  * SCALE, y + 0, x + 110 * SCALE, COLOR_EDGE_ACTIVE);
        drawhline( x + 65  * SCALE, y + 1, x + 110 * SCALE, COLOR_EDGE_ACTIVE);
    }
    drawvline( x + 110 * SCALE, y + 0 * SCALE, y + 15  * SCALE, COLOR_EDGE_NORMAL);

    /* Draw the text and bars for User interface settings */
    setcolor(!button_settings_sub_ui.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_SUBTEXT);
    drawstr(   x + 115 * SCALE, y + 5 * SCALE, USER_INTERFACE);
    if (panel_settings_ui.disabled) {
        drawhline( x + 110 * SCALE, y + 15 * SCALE, x + 175 * SCALE, COLOR_EDGE_NORMAL);
    } else {
        drawhline( x + 110 * SCALE, y + 0, x + 175 * SCALE, COLOR_EDGE_ACTIVE);
        drawhline( x + 110 * SCALE, y + 1, x + 175 * SCALE, COLOR_EDGE_ACTIVE);
    }
    drawvline( x + 175 * SCALE, y + 0 * SCALE, y + 15  * SCALE, COLOR_EDGE_NORMAL);

    /* Draw the text and bars for A/V settings */
    setcolor(!button_settings_sub_av.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_SUBTEXT);
    drawstr(   x + 180 * SCALE, y + 5 * SCALE, AUDIO_VIDEO);
    if (panel_settings_av.disabled) {
        drawhline( x + 175 * SCALE, y + 15 * SCALE, x + w + 0 * SCALE, COLOR_EDGE_NORMAL);
    } else {
        drawhline( x + 175 * SCALE, y + 0, x + w + 0 * SCALE, COLOR_EDGE_ACTIVE);
        drawhline( x + 175 * SCALE, y + 1, x + w + 0 * SCALE, COLOR_EDGE_ACTIVE);
    }
}

static void background_draw(PANEL *UNUSED(p), int UNUSED(x), int UNUSED(y), int width, int height){
    // Current user avatar & name background
    drawrect(0, 0, LIST_RIGHT, LIST_Y, COLOR_MENU_BACKGROUND);
    // Friend list ('roaster') background
    drawrect(0, LIST_Y, LIST_RIGHT, height + LIST_BOTTOM, COLOR_LIST_BACKGROUND);
    // Bottom icons menu background
    drawrect(0, height + LIST_BOTTOM, LIST_RIGHT, height, COLOR_MENU_BACKGROUND);

    // Current user avatar & name
    drawself();

    // Chat background
    drawrect(LIST_RIGHT, 0, width, height, COLOR_MAIN_BACKGROUND);

    // Chat and chat header separation
    extern PANEL panel_settings;
    if (panel_item[1].disabled) {
        drawhline(LIST_RIGHT, LIST_Y - 1, width, COLOR_EDGE_NORMAL);
    } else {
        drawhline(LIST_RIGHT, (LIST_Y / 2) - 1, width, COLOR_EDGE_NORMAL);
    }
}

static _Bool background_mmove(PANEL *UNUSED(p), int UNUSED(x), int UNUSED(y), int UNUSED(width), int UNUSED(height), int UNUSED(mx), int UNUSED(my), int UNUSED(dx), int UNUSED(dy))
{
    return 0;
}

static _Bool background_mdown(PANEL *UNUSED(p))
{
    return 0;
}

static _Bool background_mright(PANEL *UNUSED(p))
{
    return 0;
}

static _Bool background_mwheel(PANEL *UNUSED(p), int UNUSED(height), double UNUSED(d))
{
    return 0;
}

static _Bool background_mup(PANEL *UNUSED(p))
{
    return 0;
}

static _Bool background_mleave(PANEL *UNUSED(p))
{
    return 0;
}

// Scrollbar or friend list
SCROLLABLE scroll_list = {
    .panel = {
        .type = PANEL_SCROLLABLE,
    },
    .color = C_SCROLL,
    .x = 2,
    .left = 1,
},

// Scrollbar in chat window
scroll_friend = {
    .panel = {
        .type = PANEL_SCROLLABLE,
    },
    .color = C_SCROLL,
},

// ?
// @TODO
scroll_group = {
    .panel = {
        .type = PANEL_SCROLLABLE,
    },
    .color = C_SCROLL,
},

// Color is not used for settings
// @TODO
scroll_settings = {
    .panel = {
        .type = PANEL_SCROLLABLE,
    },
    .color = C_SCROLL,
};

/* */
MESSAGES messages_friend = {
    .panel = {
        .type = PANEL_MESSAGES,
        .content_scroll = &scroll_friend,
    }
},

messages_group = {
    .panel = {
        .type = PANEL_MESSAGES,
        .content_scroll = &scroll_group,
    },
    .type = 1
};

PANEL panel_list = {
    .type = PANEL_LIST,
    .content_scroll = &scroll_list,
},

/* Panel to draw settings page */
panel_settings_utox = {
    .drawfunc = drawsettings_text_utox,
    .content_scroll = &scroll_settings,
    .child = (PANEL*[]) {
        (void*)&edit_name,
        (void*)&edit_status,
        // Text: Tox ID
        (void*)&edit_toxid,
        (void*)&button_copyid,
        // User's tox id
        #ifdef EMOJI_IDS
        (void*)&button_change_id_type,
        #endif
        (void*)&dropdown_language,
        NULL
    }
},

panel_settings_net = {
    .drawfunc = drawsettings_text_network,
    .content_scroll = &scroll_settings,
    .child = (PANEL*[]) {
        (void*)&edit_proxy_ip,
        (void*)&edit_proxy_port,
        (void*)&dropdown_proxy,
        (void*)&dropdown_ipv6,
        (void*)&dropdown_udp,
        NULL
    },
    /* Disabled by default, enabled by network button */
    .disabled = 1
},

panel_settings_ui = {
    .drawfunc = drawsettings_text_ui,
    .content_scroll = &scroll_settings,
    .child = (PANEL*[]) {
        (void*)&dropdown_dpi,
        (void*)&dropdown_theme,
        (void*)&dropdown_logging,
        (void*)&dropdown_close_to_tray, (void*)&dropdown_start_in_tray,
        (void*)&dropdown_auto_startup,
        (void*)&dropdown_typing_notes,
        NULL
    },
    .disabled = 1
},

panel_settings_av = {
    .drawfunc = drawsettings_text_av,
    .content_scroll = &scroll_settings,
    .child = (PANEL*[]) {
        (void*)&button_callpreview,
        (void*)&button_videopreview,
        (void*)&dropdown_audio_in,
        (void*)&dropdown_audio_out,
        (void*)&dropdown_video,
        (void*)&dropdown_audible_notification,
        (void*)&dropdown_audible_notification_messages,
        (void*)&dropdown_audio_filtering,
        NULL
    },
    .disabled = 1
},

panel_settings = {
    .type = PANEL_NONE,
    .drawfunc = drawsettings_sub_header,
    .child = (PANEL*[]) {
        (void*)&button_settings_sub_utox,
        (void*)&button_settings_sub_net,
        (void*)&button_settings_sub_ui,
        (void*)&button_settings_sub_av,
        (void*)&scroll_settings,
        (void*)&panel_settings_utox,
        (void*)&panel_settings_net,
        (void*)&panel_settings_ui,
        (void*)&panel_settings_av,
        NULL
    }
},

panel_item[] = {
    {
        .type = PANEL_NONE,
        //.disabled = 1,
        .drawfunc = drawadd,
        .child = (PANEL*[]) {
            (void*)&button_addfriend,
            (void*)&edit_addid, (void*)&edit_addmsg,
            NULL
        }
    },

    {
        .type = PANEL_NONE,
        .disabled = 1,
        .drawfunc = drawsettings_header,
        .child = (PANEL*[]) {
            &panel_settings,
            NULL
        }
    },

    {
        .type = PANEL_NONE,
        .disabled = 1,
        .drawfunc = drawtransfer,
        .child = (PANEL*[]) {
            NULL
        }
    },

    {
        .type = PANEL_NONE,
        .disabled = 1,
        .drawfunc = drawfriend,
        .child = (PANEL*[]) {
            (void*)&button_call, (void*)&button_video, (void*)&button_sendfile,
            (void*)&button_chat1, (void*)&button_chat2, (void*)&button_chat_send,
            (void*)&edit_msg,
            (void*)&scroll_friend,
            (void*)&messages_friend,
            NULL
        }
    },

    {
        .type = PANEL_NONE,
        .disabled = 1,
        .drawfunc = drawgroup,
        .child = (PANEL*[]) {
            (void*)&button_group_audio,
            (void*)&edit_msg_group,
            (void*)&scroll_group,
            (void*)&messages_group,
            (void*)&button_chat_send,
            NULL
        }
    },

    {
        .type = PANEL_NONE,
        .disabled = 1,
        .drawfunc = drawfriendreq,
        .child = (PANEL*[]) {
            (void*)&button_acceptfriend,
            NULL
        }
    },
},

/* Side panel probably the chat window */
panel_side = {
    .type = PANEL_NONE,
    .child = (PANEL*[]) {
        &panel_item[0], &panel_item[1], &panel_item[2], &panel_item[3], &panel_item[4], &panel_item[5], NULL
    }
},

/* Main window structure */
panel_main = {
    .type = PANEL_MAIN,
    .child = (PANEL*[]) {
        (void*)&button_add, (void*)&button_groups, (void*)&button_transfer, (void*)&button_settings,
        (void*)&button_avatar, (void*)&button_name, (void*)&button_statusmsg, (void*)&button_status,
        &panel_list, &panel_side,
        (void*)&scroll_list,
        (void*)&edit_search, (void*)&dropdown_filter,
        NULL
    }
};

void ui_scale(uint8_t scale)
{
    if(SCALE == scale) {
        return;
    }

    SCALE = scale;

    list_scale();

    panel_side.x = LIST_RIGHT;

    panel_settings.y      = LIST_Y / 2;
    panel_settings_utox.y = 16 * SCALE;
    panel_settings_net.y  = 16 * SCALE;
    panel_settings_ui.y   = 16 * SCALE;
    panel_settings_av.y   = 16 * SCALE;

    panel_list.y = LIST_Y2;
    panel_list.width = LIST_RIGHT + 1;
    panel_list.height = LIST_BOTTOM;

    messages_friend.panel.y = LIST_Y;
    messages_friend.panel.height = MESSAGES_BOTTOM;
    messages_friend.panel.width = -SCROLL_WIDTH;

    messages_group.panel.y = LIST_Y;
    messages_group.panel.height = MESSAGES_BOTTOM;
    messages_group.panel.width = -SCROLL_WIDTH;

    scroll_settings.panel.y = 16 * SCALE;
    scroll_settings.content_height = 150 * SCALE;

    scroll_group.panel.y = LIST_Y;
    scroll_group.panel.height = MESSAGES_BOTTOM;

    scroll_friend.panel.y = LIST_Y;
    scroll_friend.panel.height = MESSAGES_BOTTOM;

    scroll_list.panel.y = LIST_Y2;
    scroll_list.panel.width = LIST_RIGHT + 1;
    scroll_list.panel.height = LIST_BOTTOM;


    PANEL b_add = {
        .type = PANEL_BUTTON,
        .x = 0,
        .y = LIST_BOTTOM,
        .width = SCALE * 27,
        .height = -LIST_BOTTOM,
    },

    b_groups = {
        .type = PANEL_BUTTON,
        .x = SCALE * 28 * 1,
        .y = LIST_BOTTOM,
        .width = SCALE * 27,
        .height = -LIST_BOTTOM,
    },

    b_transfer = {
        .type = PANEL_BUTTON,
        .x = SCALE * 28 * 2,
        .y = LIST_BOTTOM,
        .width = SCALE * 27,
        .height = -LIST_BOTTOM,
    },

    b_settings = {
        .type = PANEL_BUTTON,
        .x = SCALE * 28 * 3,
        .y = LIST_BOTTOM,
        .width = SCALE * 27,
        .height = -LIST_BOTTOM,
    },

    b_copyid = {
        .type = PANEL_BUTTON,
        .x = SCALE * 33,
        .y = SCALE * 53,
        .width = BM_SBUTTON_WIDTH,
        .height = BM_SBUTTON_HEIGHT,
    },

    b_settings_sub_utox = {
        .type   = PANEL_BUTTON,
        .x      = 1  * SCALE, /* Nudged 1px as a buffer */
        .y      = 1  * SCALE,
        .width  = 64 * SCALE, /* Nudged 1px as a buffer */
        .height = 14 * SCALE,
    },

    b_settings_sub_net = {
        .type   = PANEL_BUTTON,
        .x      = 66 * SCALE, /* Nudged 1px as a buffer */
        .y      = 1  * SCALE,
        .width  = 44 * SCALE,
        .height = 14 * SCALE,
    },

    b_settings_sub_ui = {
        .type   = PANEL_BUTTON,
        .x      = 111 * SCALE, /* Nudged 1px as a buffer */
        .y      = 1   * SCALE,
        .width  = 64  * SCALE, /* Nudged 1px as a buffer */
        .height = 14  * SCALE,
    },

    b_settings_sub_av = {
        .type   = PANEL_BUTTON,
        .x      = 176 * SCALE, /* Nudged 1px as a buffer */
        .y      = 1   * SCALE,
        .width  = 400 * SCALE, /* Fill the rest of the space for this button */
        .height = 14  * SCALE,
    },

    #ifdef EMOJI_IDS
    b_change_id_type = {
        .type = PANEL_BUTTON,
        .x = SCALE * 80,
        .y = SCALE * 53,
        .width = BM_SBUTTON_WIDTH,
        .height = BM_SBUTTON_HEIGHT,
    },
    #endif

    b_addfriend = {
        .type = PANEL_BUTTON,
        .x = -SCALE * 5 - BM_SBUTTON_WIDTH,
        .y = LIST_Y + SCALE * 84,
        .width = BM_SBUTTON_WIDTH,
        .height = BM_SBUTTON_HEIGHT,
    },

    b_call = {
        .type = PANEL_BUTTON,
        .x = -62 * SCALE,
        .y = 5 * SCALE,
        .width = BM_LBUTTON_WIDTH,
        .height = BM_LBUTTON_HEIGHT,
    },

    b_group_audio = {
        .type = PANEL_BUTTON,
        .x = -31 * SCALE,
        .y = 5 * SCALE,
        .width = BM_LBUTTON_WIDTH,
        .height = BM_LBUTTON_HEIGHT,
    },

    b_video = {
        .type = PANEL_BUTTON,
        .x = -31 * SCALE,
        .y = 5 * SCALE,
        .width = BM_LBUTTON_WIDTH,
        .height = BM_LBUTTON_HEIGHT,
    },

    b_sendfile = {
        .type = PANEL_BUTTON,
        .x = -93 * SCALE,
        .y = 5 * SCALE,
        .width = BM_LBUTTON_WIDTH,
        .height = BM_LBUTTON_HEIGHT,
    },

    b_acceptfriend = {
        .type = PANEL_BUTTON,
        .x = SCALE * 5,
        .y = LIST_Y + SCALE * 5,
        .width = BM_SBUTTON_WIDTH,
        .height = BM_SBUTTON_HEIGHT,
    },

    b_callpreview = {
        .type   = PANEL_BUTTON,
        .x      = 5   * SCALE,
        .y      = 125 * SCALE,
        .width  = BM_LBUTTON_WIDTH,
        .height = BM_LBUTTON_HEIGHT,
    },

    b_videopreview = {
        .type   = PANEL_BUTTON,
        .x      = 35  * SCALE,
        .y      = 125 * SCALE,
        .width  = BM_LBUTTON_WIDTH,
        .height = BM_LBUTTON_HEIGHT,
    },

    /* top right chat message window button */
    b_chat1 = {
        .type = PANEL_BUTTON,
        .x = -40 * SCALE - BM_CHAT_BUTTON_WIDTH,
        .y = -40 * SCALE,
        .height = BM_CHAT_BUTTON_HEIGHT,
        .width = BM_CHAT_BUTTON_WIDTH,
    },

    /* bottom right chat message window button */
    b_chat2 = {
        .type = PANEL_BUTTON,
        .x = -40 * SCALE - BM_CHAT_BUTTON_WIDTH,
        .y = -40 * SCALE + BM_CHAT_BUTTON_HEIGHT + SCALE,
        .height = BM_CHAT_BUTTON_HEIGHT + SCALE,
        .width = BM_CHAT_BUTTON_WIDTH,
    },

    b_chat_send = {
        .type   = PANEL_BUTTON,
        .x      = -5 * SCALE - BM_CHAT_SEND_WIDTH,
        .y      = -40 * SCALE,
        .height = BM_CHAT_SEND_HEIGHT,
        .width  = BM_CHAT_SEND_WIDTH,
    },

    b_avatar = {
        .type = PANEL_BUTTON,
        .x = SELF_AVATAR_X,
        .y = SELF_AVATAR_Y,
        .width = BM_CONTACT_WIDTH,
        .height = BM_CONTACT_WIDTH,
    },

    b_name = {
        .type = PANEL_BUTTON,
        .x = SELF_NAME_X,
        .y = SELF_NAME_Y + 2 * SCALE,
        .width = SELF_STATUS_X - SELF_NAME_X,
        .height = SELF_MSG_Y - SELF_NAME_Y,
    },

    b_statusmsg = {
        .type = PANEL_BUTTON,
        .x = SELF_MSG_X,
        .y = SELF_MSG_Y + 2 * SCALE,
        .width = SELF_STATUS_X - SELF_MSG_X,
        .height = 6 * SCALE,
    },

    b_status = {
        .type = PANEL_BUTTON,
        .x = SELF_STATUS_X,
        .y = SELF_STATUS_Y,
        .width = BM_STATUSAREA_WIDTH,
        .height = BM_STATUSAREA_HEIGHT,
    };

    button_add.panel = b_add;
    button_settings.panel = b_settings;
    button_transfer.panel = b_transfer;
    button_groups.panel = b_groups;
    button_copyid.panel = b_copyid;
    button_settings_sub_utox.panel = b_settings_sub_utox;
    button_settings_sub_net.panel = b_settings_sub_net;
    button_settings_sub_ui.panel = b_settings_sub_ui;
    button_settings_sub_av.panel = b_settings_sub_av;
    #ifdef EMOJI_IDS
    button_change_id_type.panel = b_change_id_type;
    #endif
    button_addfriend.panel = b_addfriend;
    button_call.panel = b_call;
    button_group_audio.panel = b_group_audio;
    button_video.panel = b_video;
    button_sendfile.panel = b_sendfile;
    button_acceptfriend.panel = b_acceptfriend;
    button_callpreview.panel = b_callpreview;
    button_videopreview.panel = b_videopreview;
    button_chat1.panel = b_chat1;
    button_chat2.panel = b_chat2;
    button_chat_send.panel = b_chat_send;
    button_avatar.panel = b_avatar;
    button_name.panel = b_name;
    button_statusmsg.panel = b_statusmsg;
    button_status.panel = b_status;

    PANEL d_notifications = {
        .type   = PANEL_DROPDOWN,
        .x      = 5  * SCALE,
        .y      = 15 * SCALE,
        .height = 12 * SCALE,
        .width  = 20 * SCALE
    };

    PANEL d_notifications_messages = {
        .type   = PANEL_DROPDOWN,
        .x      = 50  * SCALE,
        .y      = 15  * SCALE,
        .height = 12  * SCALE,
        .width  = 20  * SCALE
    },


    #ifdef AUDIO_FILTERING
    d_audio_filtering = {
        .type   = PANEL_DROPDOWN,
        .x      = 100 * SCALE,
        .y      = 15  * SCALE,
        .height = 12  * SCALE,
        .width  = 20  * SCALE
    },
    #endif

    d_audio_in = {
        .type   = PANEL_DROPDOWN,
        .x      = 5   * SCALE,
        .y      = 45  * SCALE,
        .height = 12  * SCALE,
        .width  = 180 * SCALE
    },

    d_audio_out = {
        .type   = PANEL_DROPDOWN,
        .x      = 5   * SCALE,
        .y      = 70  * SCALE,
        .height = 12  * SCALE,
        .width  = 180 * SCALE
    },

    d_video = {
        .type   = PANEL_DROPDOWN,
        .x      = 5   * SCALE,
        .y      = 95  * SCALE,
        .height = 12  * SCALE,
        .width  = 180 * SCALE
    },

    d_dpi = {
        .type   = PANEL_DROPDOWN,
        .x      = 75  * SCALE,
        .y      = 15  * SCALE,
        .height = 12  * SCALE,
        .width  = 100 * SCALE
    },

    d_language = {
        .type   = PANEL_DROPDOWN,
        .x      = 5   * SCALE,
        .y      = 84  * SCALE,
        .height = 12  * SCALE,
        .width  = 100 * SCALE
    },

    d_filter = {
        .type   = PANEL_DROPDOWN,
        .x      = LIST_RIGHT - SCALE * 25,
        .y      = SEARCH_Y,
        .height = 12 * SCALE,
        .width  = 25 * SCALE,
    },

    d_proxy = {
        .type   = PANEL_DROPDOWN,
        .x      = 5  * SCALE,
        .y      = 40 * SCALE,
        .height = 12 * SCALE,
        .width  = 60 * SCALE
    },

    d_ipv6 = {
        .type   = PANEL_DROPDOWN,
        .x      = 24 * SCALE,
        .y      = 13 * SCALE,
        .height = 12 * SCALE,
        .width  = 20 * SCALE
    },

    d_udp = {
        .type   = PANEL_DROPDOWN,
        .x      = 74 * SCALE,
        .y      = 13 * SCALE,
        .height = 12 * SCALE,
        .width  = 20 * SCALE
    },

    d_logging = {
        .type   = PANEL_DROPDOWN,
        .x      = 5   * SCALE,
        .y      = 39 * SCALE,
        .height = 12  * SCALE,
        .width  = 20  * SCALE
    },

    d_theme = {
        .type   = PANEL_DROPDOWN,
        .x      = 5  * SCALE,
        .y      = 15 * SCALE,
        .height = 12 * SCALE,
        .width  = 60 * SCALE
    },

    d_close_to_tray = {
        .type   = PANEL_DROPDOWN,
        .x      = 5   * SCALE,
        .y      = 63 * SCALE,
        .height = 12  * SCALE,
        .width  = 20  * SCALE
    },

    d_start_in_tray = {
        .type   = PANEL_DROPDOWN,
        .x      = 75  * SCALE,
        .y      = 63 * SCALE,
        .height = 12 * SCALE,
        .width  = 20 * SCALE
    },

    d_auto_startup = {
        .type   = PANEL_DROPDOWN,
        .x      = 5  * SCALE,
        .y      = 87 * SCALE,
        .height = 12 * SCALE,
        .width  = 20 * SCALE
    },

    d_typing_notes = {
        .type   = PANEL_DROPDOWN,
        .x      = 5   * SCALE,
        .y      = 114 * SCALE,
        .height = 12  * SCALE,
        .width  = 20  * SCALE
    };

    dropdown_audio_in.panel = d_audio_in;
    dropdown_audio_out.panel = d_audio_out;
    dropdown_video.panel = d_video;
    dropdown_dpi.panel = d_dpi;
    dropdown_language.panel = d_language;
    dropdown_filter.panel = d_filter;
    dropdown_proxy.panel = d_proxy;
    dropdown_ipv6.panel = d_ipv6;
    dropdown_udp.panel = d_udp;
    dropdown_logging.panel = d_logging;
    dropdown_audible_notification.panel = d_notifications;
    dropdown_audible_notification_messages.panel = d_notifications_messages;
    dropdown_close_to_tray.panel = d_close_to_tray;
    dropdown_start_in_tray.panel = d_start_in_tray;
    dropdown_theme.panel = d_theme;
    dropdown_auto_startup.panel = d_auto_startup;

    #ifdef AUDIO_FILTERING
    dropdown_audio_filtering.panel = d_audio_filtering;
    #endif
    dropdown_typing_notes.panel = d_typing_notes;


    PANEL e_name = {
        .type = PANEL_EDIT,
        .x = 5 * SCALE,
        .y = SCALE * 14,
        .height = 12 * SCALE,
        .width = -SCROLL_WIDTH - 5 * SCALE
    },

    e_status = {
        .type = PANEL_EDIT,
        .x = 5 * SCALE,
        .y = SCALE * 38,
        .height = 12 * SCALE,
        .width = -SCROLL_WIDTH - 5 * SCALE
    },

    e_toxid = {
        .type = PANEL_EDIT,
        .x = 3 * SCALE,
        .y = SCALE * 63,
        .height = 12 * SCALE,
        .width = -SCROLL_WIDTH - 5 * SCALE
    },

    e_addid = {
        .type = PANEL_EDIT,
        .x = 5 * SCALE,
        .y = LIST_Y + SCALE * 14,
        .height = 12 * SCALE,
        .width = -5 * SCALE
    },

    e_addmsg = {
        .type = PANEL_EDIT,
        .x = 5 * SCALE,
        .y = LIST_Y + SCALE * 38,
        .height = SCALE * 42,
        .width = -5 * SCALE,
    },

    /* Message entry box for friends and groups */
    e_msg = {
        .type   = PANEL_EDIT,
        .x      = 5 * SCALE,
        .y      = -40 * SCALE,
        // a text line is 8 high. 32 / 8 = 4 lines of text.
        .height = 32 * SCALE,
        .width  = -40 * SCALE - BM_CHAT_BUTTON_WIDTH,
    },

    e_msg_group = {
        .type   = PANEL_EDIT,
        .x      = 5 * SCALE,
        .y      = -40 * SCALE,
        // a text line is 8 high. 32 / 8 = 4 lines of text.
        .height = 32 * SCALE,
        .width  = -10 * SCALE - BM_CHAT_SEND_WIDTH,
    },

    e_search = {
        .type = PANEL_EDIT,
        .x = 0,
        .y = SEARCH_Y,
        .height = 12 * SCALE,
        .width = LIST_RIGHT - SCALE * 25,
    },

    e_proxy_ip = {
        .type   = PANEL_EDIT,
        .x      = 70 * SCALE,
        .y      = 40 * SCALE,
        .height = 12 * SCALE,
        .width  = 60 * SCALE
    },

    e_proxy_port = {
        .type   = PANEL_EDIT,
        .x      = 135 * SCALE,
        .y      = 40  * SCALE,
        .height = 12  * SCALE,
        .width  = 30  * SCALE
    };

    edit_name.panel = e_name;
    edit_status.panel = e_status;
    edit_toxid.panel = e_toxid;
    edit_addid.panel = e_addid;
    edit_addmsg.panel = e_addmsg;
    edit_msg.panel = e_msg;
    edit_msg_group.panel = e_msg_group;
    edit_search.panel = e_search;
    edit_proxy_ip.panel = e_proxy_ip;
    edit_proxy_port.panel = e_proxy_port;

    setscale();
}

/* Use the preprocessor to build functions for all user inactions
    These are functions that are (must be) defined elsewehere. The preprocessor in this case creates the prototypes that
    will then be used by panel_draw_sub to call the correct function
*/
#define FUNC(x, ret, ...) static ret (* x##func[])(void *p, ##__VA_ARGS__) = { \
    (void*)background_##x, \
    (void*)messages_##x, \
    (void*)list_##x, \
    (void*)button_##x, \
    (void*)dropdown_##x, \
    (void*)edit_##x, \
    (void*)scroll_##x, \
};

FUNC(draw, void, int x, int y, int width, int height);
FUNC(mmove, _Bool, int x, int y, int width, int height, int mx, int my, int dx, int dy);
FUNC(mdown, _Bool);
FUNC(mright, _Bool);
FUNC(mwheel, _Bool, int height, double d);
FUNC(mup, _Bool);
FUNC(mleave, _Bool);

#undef FUNC

/* Use the preprocessor to add code to adjust the x,y cords for panels or sub panels. */
#define FUNC() {\
    int relx = (p->x < 0) ? width + p->x : p->x;\
    int rely = (p->y < 0) ? height + p->y : p->y;\
    x += relx; \
    y += rely; \
    width = (p->width <= 0) ? width + p->width - relx : p->width; \
    height = (p->height <= 0) ? height + p->height - rely : p->height; }\

static void panel_update(PANEL *p, int x, int y, int width, int height)
{
    FUNC();

    if(p->type == PANEL_MESSAGES) {
        MESSAGES *m = (void*)p;
        m->width = width;
        if(!p->disabled) {
            messages_updateheight(m);
        }
    }

    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            panel_update(subp, x, y, width, height);
        }
    }
}

void ui_size(int width, int height)
{
    panel_update(&panel_main, 0, 0, width, height);
    tooltip_reset();
}

void ui_mouseleave(void)
{
    panel_mleave(&panel_main);
    tooltip_reset();
    redraw();
}

static void panel_draw_sub(PANEL *p, int x, int y, int width, int height)
{
    FUNC();

    if(p->content_scroll) {
        pushclip(x, y, width, height);
        y -= scroll_gety(p->content_scroll, height);
    }

    if(p->type) {
        drawfunc[p->type - 1](p, x, y, width, height);
    } else {
        if(p->drawfunc) {
            p->drawfunc(x, y, width, height);
        }
    }

    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            if(!subp->disabled) {
                panel_draw_sub(subp, x, y, width, height);
            }
        }
    }

    if(p->content_scroll) {
        popclip();
    }
}

void panel_draw(PANEL *p, int x, int y, int width, int height)
{
    FUNC();

    //pushclip(x, y, width, height);

    if(p->type) {
        drawfunc[p->type - 1](p, x, y, width, height);
    } else {
        if(p->drawfunc) {
            p->drawfunc(x, y, width, height);
        }
    }

    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            if(!subp->disabled) {
                panel_draw_sub(subp, x, y, width, height);
            }
        }
    }

    //popclip();

    dropdown_drawactive();
    contextmenu_draw();
    tooltip_draw();

    enddraw(x, y, width, height);
}

_Bool panel_mmove(PANEL *p, int x, int y, int width, int height, int mx, int my, int dx, int dy)
{
    if(p == &panel_main) {
        mouse.x = mx;
        mouse.y = my;
    }

    mx -= (p->x < 0) ? width + p->x : p->x;
    my -= (p->y < 0) ? height + p->y : p->y;
    FUNC();

    int mmy = my;

    if(p->content_scroll) {
        int scroll_y = scroll_gety(p->content_scroll, height);
        if(my < 0) {
            mmy = -1;
        } else if (my >= height) {
            mmy = 1024 * 1024 * 1024;//large value
        } else {
            mmy = my + scroll_y;
        }
        y -= scroll_y;
        my += scroll_y;
    }

    _Bool draw = p->type ? mmovefunc[p->type - 1](p, x, y, width, height, mx, mmy, dx, dy) : 0;
    // Has to be called before children mmove
    if(p == &panel_main) {
        draw |= tooltip_mmove();
    }
    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            if(!subp->disabled) {
                draw |= panel_mmove(subp, x, y, width, height, mx, my, dx, dy);
            }
        }
    }

    if(p == &panel_main) {
        draw |= contextmenu_mmove(mx, my, dx, dy);
        if(draw) {
            redraw();
        }
    }

    return draw;
}

static _Bool panel_mdown_sub(PANEL *p)
{
    if(p->type && mdownfunc[p->type - 1](p)) {
        return 1;
    }

    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            if(!subp->disabled) {
                if(panel_mdown_sub(subp)) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

void panel_mdown(PANEL *p)
{
    if(contextmenu_mdown() || tooltip_mdown()) {
        redraw();
        return;
    }

    _Bool draw = edit_active();
    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            if(!subp->disabled) {
                if(panel_mdown_sub(subp)) {
                    draw = 1;
                    break;
                }
            }
        }
    }

    if(draw) {
        redraw();
    }
}

_Bool panel_dclick(PANEL *p, _Bool triclick)
{
    _Bool draw = 0;
    if(p->type == PANEL_EDIT) {
        draw = edit_dclick((EDIT*)p, triclick);
    } else if(p->type == PANEL_MESSAGES) {
        draw = messages_dclick((MESSAGES*)p, triclick);
    }

    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            if(!subp->disabled) {
                draw = panel_dclick(subp, triclick);
                if(draw) {
                    break;
                }
            }
        }
    }

    if(draw && p == &panel_main) {
        redraw();
    }

    return draw;
}

_Bool panel_mright(PANEL *p)
{
    _Bool draw = p->type ? mrightfunc[p->type - 1](p) : 0;
    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            if(!subp->disabled) {
                draw |= panel_mright(subp);
            }
        }
    }

    if(draw && p == &panel_main) {
        redraw();
    }

    return draw;
}

_Bool panel_mwheel(PANEL *p, int x, int y, int width, int height, double d)
{
    FUNC();

    _Bool draw = p->type ? mwheelfunc[p->type - 1](p, height, d) : 0;
    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            if(!subp->disabled) {
                draw |= panel_mwheel(subp, x, y, width, height, d);
            }
        }
    }

    if(draw && p == &panel_main) {
        redraw();
    }

    return draw;
}

_Bool panel_mup(PANEL *p)
{
    _Bool draw = p->type ? mupfunc[p->type - 1](p) : 0;
    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            if(!subp->disabled) {
                draw |= panel_mup(subp);
            }
        }
    }

    if(p == &panel_main) {
        draw |= contextmenu_mup();
        tooltip_mup();
        if(draw) {
            redraw();
        }
    }

    return draw;
}

_Bool panel_mleave(PANEL *p)
{
    _Bool draw = p->type ? mleavefunc[p->type - 1](p) : 0;
    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            if(!subp->disabled) {
                draw |= panel_mleave(subp);
            }
        }
    }

    if(p == &panel_main) {
        draw |= contextmenu_mleave();
        if(draw) {
            redraw();
        }
    }

    return draw;
}
