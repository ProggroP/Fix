#include "pebble.h"

// ---------------------------------------------------------------------------
// Schriftgroessen, Ringabstaende, Kastenbreiten
//
// Emery und Gabbro haben deutlich mehr Pixel als die uebrigen Modelle, aber
// FONT_KEY_BITHAM_42_LIGHT ist bereits die groesste helle Systemschrift des
// Systems -- mit Bordmitteln lassen sich die Stunden dort also gar nicht
// vergroessern. Auf diesen beiden Plattformen kommen deshalb eigene
// Roboto-Schnitte als Resource dazu. Alle anderen Plattformen behalten exakt
// die bisherigen Systemschriften und Abstaende.
// ---------------------------------------------------------------------------

#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
  #define BIG_SCREEN

  #define SZ_HOUR      58   // Stundenzahlen
  #define SZ_BIGM      52   // Kasten der "grossen Minuten" (Modus 7), Schrift 44
  #define SZ_MINS      36   // Minuten
  #define SZ_SECS      25   // Sekunden
  // Die beiden Datumszeilen mit Wochentag bzw. Monat bleiben bei der
  // Systemschrift, und zwar aus Platzgruenden: die Datumsreihe traegt fuenf
  // Eintraege im Abstand von 60 Grad, zwei davon stehen zeitweise auf gleicher
  // Hoehe nebeneinander -- ihr Abstand betraegt dann genau einen Ringradius.
  // Eine breitere Zeile wuerde dort mit der Nachbarzeile und rechts mit den
  // Stundenzahlen zusammenstossen. Der reine Tag in Modus 6 ist kurz genug.
  #define SZ_DATE3     18   // Datum "Wo Mon Tg"
  #define SZ_DATE45    24   // Datum "Wo Tg" bzw. "Mon Tg"
  #define SZ_DATE6     25   // Datum "Tg"

  // Kastenbreite. Zu schmal heisst abgeschnittener Text mit "...", und die
  // Kastenhoehe muss ausserdem zur geladenen Schriftgroesse passen.
  #define W_DATE3      80
  #define W_DATE45     60

  // Abstand der Ringe zum Stundenring. Auch diese Werte sind gegen Basalt
  // ausgemessen: dort stehen Sekunden, Minuten und Stunde mit 17 und 19
  // Pixeln Abstand nebeneinander, Tag und Minuten mit 16 und 18. Mit den
  // groesseren Schriften sind daraus rund 22 und 26 Pixel geworden.
  #define OFF_MINS     60
  #define OFF_SECS    111
  #define OFF_DATE3    55
  #define OFF_DATE45   45
  #define OFF_DATE6   109
  #define OFF_BIGM     45
  #define SHIFT_BIGM   26   // Modus 7 rueckt den Ring zusaetzlich zur Seite

  // Abstand des Zeigerstrichs zur jeweils innersten Zahlenreihe. Die Werte
  // sind gegen Basalt ausgemessen (24-Stunden-Anzeige, also zweistellige
  // Stunde als breitester Fall) und auf dieselbe Luecke gebracht, bei den
  // vergroesserten Schriften um denselben Faktor mitskaliert. Basalt zeigt
  // je Modus 2, 9, 1, 3, 13, 11, 1 und 1 Pixel.
  #define GAP_HOUR     32
  #define GAP_MINS     34
  #define GAP_SECS     23
  #define GAP_DATE3    38
  #define GAP_DATE45   39
  #define GAP_DATE6    18
  #define GAP_BIGM     24

  // Laenge des Reststuecks am gegenueberliegenden Rand. Es liegt fest am
  // Displayrand, die Zahlenreihe dagegen bei drei Vierteln der Breite --
  // auf dem breiten Gabbro klafft dazwischen sonst die doppelte Luecke.
  #if defined(PBL_PLATFORM_EMERY)
    #define STUB_LEN   11
  #else
    #define STUB_LEN   28
  #endif
#else
  #define SZ_HOUR      42
  #define SZ_BIGM      42
  #define SZ_MINS      28
  #define SZ_SECS      18
  #define SZ_DATE3     18
  #define SZ_DATE45    24
  #define SZ_DATE6     18

  #define W_DATE3      80
  #define W_DATE45     60

  #define OFF_MINS     40
  #define OFF_SECS     70
  #define OFF_DATE3    55
  #define OFF_DATE45   45
  #define OFF_DATE6    70
  #define OFF_BIGM     35
  #define SHIFT_BIGM   20

  #define STUB_LEN     11
#endif

// Mittelpunkt der Zahlenscheiben. Er liegt seitlich ausserhalb des Displays,
// und zwar genau so weit, wie der Radius ueber ein Viertel der Displaybreite
// hinausgeht -- dadurch sitzt die Zahlenreihe immer bei drei Vierteln der
// Breite, unabhaengig von diesem Wert. Was er steuert, ist allein die
// Kruemmung: je groesser, desto flacher der Bogen, desto gerader steht die
// Zahlenreihe und desto weiter liegen die Zeilen auseinander.
//
// Der Wert muss je Plattform anders ausfallen, weil der Radius mit einem
// Viertel der Displaybreite waechst: bei gleichem Versatz bekaeme Gabbro
// allein wegen seiner 60 Pixel Mehrbreite einen um 15 Pixel groesseren
// Radius. Massgeblich ist aber nicht die Breite, sondern die Hoehe, ueber
// die sich der Bogen kruemmt -- und die ist bei beiden fast gleich: Emery
// 228 Pixel, Gabbro auf Hoehe der Zahlenreihe rund 225 (der runde Rand
// schneidet oben und unten ab). Beide Werte laufen deshalb auf denselben
// Radius von gut 180 Pixeln hinaus.
#if defined(PBL_PLATFORM_EMERY)
  #define ARC_OFFSET     130   // Radius 180 bei 200 Pixel Breite
#elif defined(PBL_PLATFORM_GABBRO)
  #define ARC_OFFSET     120   // Radius 185 bei 260 Pixel Breite
#else
  #define ARC_OFFSET      80
#endif

// Der Ring rollt in Schritten von 30 Grad. Im 5-Minuten-Raster steht ein
// Schritt fuer fuenf Minuten, im Exaktmodus fuer eine einzelne -- die Optik
// bleibt dadurch in beiden Faellen dieselbe. Zwoelf Schritte sind eine volle
// Umdrehung: die Scheibe traegt deshalb in beiden Faellen zwoelf
// Beschriftungen, und der Punkt, an dem eine Zahl durch die naechste ersetzt
// wird, liegt genau gegenueber der Markierung -- links, weit neben dem
// Display. Mit weniger Beschriftungen taucht oben stattdessen unvermittelt
// eine neue Zahl mitten im Bild auf.
#define MIN_STEP_DEG      30
#define MIN_SLOTS         12
#define EXACT_MIN_FIRST   -5   // erste gezeichnete Minute relativ zur aktuellen

static Window *s_window;
static Layer *s_hands_layer, *s_dot_layer;
static int s_screen_w;

// Schriften je Anzeigeelement. Auf grossen Displays zeigen mehrere davon auf
// dieselbe geladene Resource, deshalb die getrennten Handles weiter unten.
static GFont s_font_hour, s_font_bigm;
static GFont s_font_mins,  s_font_mins_bold;
static GFont s_font_secs,  s_font_secs_bold;
static GFont s_font_date3, s_font_date3_bold;
static GFont s_font_date45, s_font_date45_bold;
static GFont s_font_date6, s_font_date6_bold;

#ifdef BIG_SCREEN
// Nur diese sechs werden wirklich geladen und am Ende wieder freigegeben.
static GFont s_res_hour, s_res_bigm, s_res_mid, s_res_mid_bold, s_res_sml, s_res_sml_bold;
#endif


typedef struct ClaySettings {
  GColor backcolor;
  GColor hourcolor;
  GColor minscolor;
  GColor secscolor;
  GColor datecolor;
  GColor linecolor;
  int    timemode;
  int    twentyfour;
  int    flipscreen;
  int    rotation;
  int    boldmins;
  int    boldsecs;
  int    bolddate;
  int    fixeddate;
  int    exactmins;   // neue Felder immer ans Ende, sonst passt der bereits
                      // gespeicherte Datensatz nicht mehr
} ClaySettings;

// An instance of the struct
static ClaySettings settings;

// Persistent storage key
#define SETTINGS_KEY 1

// Adjust date by a number of days +/-
void DatePlusDays( struct tm* date, int days )
{
    const time_t ONE_DAY = 24 * 60 * 60 ;

    // Seconds since start of epoch
    time_t date_seconds = mktime( date ) + (days * ONE_DAY) ;

    // Update caller's date
    // Use localtime because mktime converts to UTC so may change date
    *date = *localtime( &date_seconds ) ; ;
}

static void handle_time_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_hands_layer);
}

// Text mittig um einen Punkt setzen. Der Versatz um ein Sechstel der
// Kastenhoehe gleicht den Innenabstand der Schriften aus -- genau so stand es
// vorher an jedem einzelnen graphics_draw_text.
static void draw_centered(GContext *ctx, const char *text, GFont font,
                          int box_w, int box_h, GPoint at) {
#ifdef BIG_SCREEN
  // Die inneren Ringe liegen hier so weit innen, dass eine Beschriftung
  // seitlich komplett neben dem Display landen kann. Ohne diese Sperre ragt
  // von ihr nur ein "..." in den Rand.
  if( at.x < 0 || at.x >= s_screen_w ) {
    return;
  }
#endif
  graphics_draw_text(ctx, text, font,
                     GRect(at.x - box_w/2, at.y - box_h/2 - box_h/6, box_w, box_h),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

// Alle Schriften einmal beim Start aufloesen -- fonts_load_custom_font bei
// jedem Zeichenvorgang aufzurufen waere pure Verschwendung.
static void load_fonts() {
#ifdef BIG_SCREEN
  s_res_hour     = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_ROBOTO_LIGHT_58));
  s_res_bigm     = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_ROBOTO_MEDIUM_44));
  s_res_mid      = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_ROBOTO_LIGHT_36));
  s_res_mid_bold = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_ROBOTO_MEDIUM_36));
  s_res_sml      = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_ROBOTO_LIGHT_25));
  s_res_sml_bold = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_ROBOTO_MEDIUM_25));

  s_font_hour        = s_res_hour;
  s_font_bigm        = s_res_bigm;
  s_font_mins        = s_res_mid;
  s_font_mins_bold   = s_res_mid_bold;
  s_font_secs        = s_res_sml;
  s_font_secs_bold   = s_res_sml_bold;
  s_font_date6       = s_res_sml;
  s_font_date6_bold  = s_res_sml_bold;
  // siehe Kommentar bei SZ_DATE3: diese beiden Zeilen bleiben Systemschrift
  s_font_date3       = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  s_font_date3_bold  = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  s_font_date45      = fonts_get_system_font(FONT_KEY_GOTHIC_24);
  s_font_date45_bold = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
#else
  s_font_hour        = fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT);
  s_font_bigm        = fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS);
  s_font_mins        = fonts_get_system_font(FONT_KEY_GOTHIC_28);
  s_font_mins_bold   = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  s_font_secs        = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  s_font_secs_bold   = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  s_font_date3       = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  s_font_date3_bold  = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  s_font_date45      = fonts_get_system_font(FONT_KEY_GOTHIC_24);
  s_font_date45_bold = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  s_font_date6       = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  s_font_date6_bold  = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
#endif
}

static void unload_fonts() {
#ifdef BIG_SCREEN
  fonts_unload_custom_font(s_res_hour);
  fonts_unload_custom_font(s_res_bigm);
  fonts_unload_custom_font(s_res_mid);
  fonts_unload_custom_font(s_res_mid_bold);
  fonts_unload_custom_font(s_res_sml);
  fonts_unload_custom_font(s_res_sml_bold);
#endif
}

// Initialize the default settings
static void default_settings() {
  settings.backcolor  = GColorBlack;
  settings.hourcolor  = GColorWhite;
  settings.minscolor  = GColorWhite;
  settings.secscolor  = GColorWhite;
  settings.datecolor  = GColorWhite;
  settings.linecolor  = GColorWhite;
  settings.timemode   = 0;
  settings.twentyfour = 0;
  settings.flipscreen = 0;
  settings.rotation   = 0;
  settings.boldmins   = 0;
  settings.boldsecs   = 0;
  settings.bolddate   = 0;
  settings.fixeddate  = 0;
  settings.exactmins  = 0;
}

// Read settings from persistent storage
static void load_settings() {
  // Load the default settings
  default_settings();

  // Read settings from persistent storage, if they exist
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));

}

// Save the settings to persistent storage
static void save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void in_received_handler(DictionaryIterator *iter, void *context) {

  // read background color setting
  Tuple *backcolor_conf = dict_find(iter, MESSAGE_KEY_backcolor);
  if ( backcolor_conf ) {
    settings.backcolor = GColorFromHEX(backcolor_conf->value->int32);
    window_set_background_color(s_window, settings.backcolor);
  }

  // read hour color setting
  Tuple *hourcolor_conf = dict_find(iter, MESSAGE_KEY_hourcolor);
  if ( hourcolor_conf ) {
    settings.hourcolor = GColorFromHEX(hourcolor_conf->value->int32);
  }

  // read mins color setting
  Tuple *minscolor_conf = dict_find(iter, MESSAGE_KEY_minscolor);
  if ( minscolor_conf ) {
    settings.minscolor = GColorFromHEX(minscolor_conf->value->int32);
  }

  // read secs color setting
  Tuple *secscolor_conf = dict_find(iter, MESSAGE_KEY_secscolor);
  if ( secscolor_conf ) {
    settings.secscolor = GColorFromHEX(secscolor_conf->value->int32);
  }

  // read date color setting
  Tuple *datecolor_conf = dict_find(iter, MESSAGE_KEY_datecolor);
  if ( datecolor_conf ) {
    settings.datecolor = GColorFromHEX(datecolor_conf->value->int32);
  }

  // read line color setting
  Tuple *linecolor_conf = dict_find(iter, MESSAGE_KEY_linecolor);
  if ( linecolor_conf ) {
    settings.linecolor = GColorFromHEX(linecolor_conf->value->int32);
  }


  // read time mode setting
  Tuple *timemode_conf = dict_find(iter, MESSAGE_KEY_timemode);
  if( timemode_conf ) {
    char buf[10];
    strcpy( buf, timemode_conf->value->cstring);
    settings.timemode = atoi(buf);
    if( settings.timemode == 2 ) {
      tick_timer_service_unsubscribe();
      tick_timer_service_subscribe(SECOND_UNIT, handle_time_tick );
    }
    else
    {
      tick_timer_service_unsubscribe();
      tick_timer_service_subscribe(MINUTE_UNIT, handle_time_tick );
    }
  }

  // read 24 hour setting
  Tuple *twentyfour_conf = dict_find(iter, MESSAGE_KEY_twentyfour);
  if( twentyfour_conf ) {
    settings.twentyfour = twentyfour_conf->value->int32;
  }

  // read flip screen setting
  Tuple *flipscreen_conf = dict_find(iter, MESSAGE_KEY_flipscreen);
  if( flipscreen_conf ) {
    settings.flipscreen = flipscreen_conf->value->int32;
  }

  // read rotation setting
  Tuple *rotation_conf = dict_find(iter, MESSAGE_KEY_rotation);
  if( rotation_conf ) {
    settings.rotation = rotation_conf->value->int32;
  }

  // read bold minutes setting
  Tuple *boldmins_conf = dict_find(iter, MESSAGE_KEY_boldmins);
  if( boldmins_conf ) {
    settings.boldmins = boldmins_conf->value->int32;
  }

  // read bold seconds setting
  Tuple *boldsecs_conf = dict_find(iter, MESSAGE_KEY_boldsecs);
  if( boldsecs_conf ) {
    settings.boldsecs = boldsecs_conf->value->int32;
  }

  // read bold date setting
  Tuple *bolddate_conf = dict_find(iter, MESSAGE_KEY_bolddate);
  if( bolddate_conf ) {
    settings.bolddate = bolddate_conf->value->int32;
  }

  // read fixed date setting
  Tuple *fixeddate_conf = dict_find(iter, MESSAGE_KEY_fixeddate);
  if( fixeddate_conf ) {
    settings.fixeddate = fixeddate_conf->value->int32;
  }

  // read exact minutes setting
  Tuple *exactmins_conf = dict_find(iter, MESSAGE_KEY_exactmins);
  if( exactmins_conf ) {
    settings.exactmins = exactmins_conf->value->int32;
  }

  save_settings();

  layer_mark_dirty( s_dot_layer );
  layer_mark_dirty( s_hands_layer );

}

static void hands_update_proc(Layer *layer, GContext *ctx) {

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  int hour  = t->tm_hour;
  int min   = t->tm_min;
  int sec   = t->tm_sec;

  // debug
  //settings.timemode   = 3;
  //settings.twentyfour = 1;
  //settings.flipscreen = 0;
  //settings.rotation   = 1;
  //settings.fixeddate = 1;
  //hour = 23;
  //min  = 0;
  //sec  = 0;

  int angle = 0;
  int intangle = 0;

  GRect bounds  = layer_get_bounds(s_hands_layer);
  GPoint center = GPoint( bounds.size.w/2, bounds.size.h/2);

  graphics_context_set_antialiased(ctx, true);

  int radius = ARC_OFFSET + (bounds.size.w/4);
  GPoint bigcenter;
  if( settings.flipscreen == 0 ) {
    bigcenter = GPoint( center.x-ARC_OFFSET, center.y);
  }
  else {
    bigcenter = GPoint( center.x+ARC_OFFSET, center.y);
  }

  // hour numbers
  graphics_context_set_text_color(ctx, settings.hourcolor );
  int outer_pos = radius;
  GPoint outer;

  const char *hnums[60] =
  {
     "0",  "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11",
     "12", "13", "14",  "15",  "16",  "17",  "18",  "19",  "20",  "21",  "22",  "23",
     "0",  "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11",
     "12", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11",
     "12",  "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11",
  };

  if( settings.twentyfour == 1 ) {
    if( hour == 0 ) { hour = 24; }
    if( hour == 1 ) { hour = 25; }
  }
  else {
    hour = hour%12 + 36;
  }
  int y = 0;
  for( int i = hour-2; i < hour+3; i++ ) {
    if( settings.flipscreen == 0 ) {
      if( settings.rotation == 0 ) {
        intangle = 150-(y*30)+(min*5/10);
      }
      else {
        intangle = 30+(y*30)-(min*5/10);
      }
    }
    else {
      if( settings.rotation == 0 ) {
        intangle = 210+(y*30)-(min*5/10);
      }
      else {
        intangle = 330-(y*30)+(min*5/10);
      }
    }
    y ++;
    intangle = intangle % 360;
    angle = DEG_TO_TRIGANGLE(intangle);
    outer.y = (int16_t)(-cos_lookup(angle) * (int32_t)(outer_pos) / TRIG_MAX_RATIO) + bigcenter.y;
    outer.x = (int16_t)(sin_lookup(angle) * (int32_t)(outer_pos) / TRIG_MAX_RATIO) + bigcenter.x;
    // draw hour number
    draw_centered(ctx, hnums[i], s_font_hour, 2*SZ_HOUR, SZ_HOUR, outer);
  }

  // Minutes
  if( settings.timemode == 1 || settings.timemode == 2 || settings.timemode == 6 || settings.timemode == 7 ) {
    // Minute numbers
    graphics_context_set_text_color(ctx, settings.minscolor );

    GFont mfont;
    int   mbox;
    if( settings.timemode == 7 ) {
      if( settings.flipscreen == 0 ) {
        bigcenter = GPoint( bigcenter.x-SHIFT_BIGM, bigcenter.y);
      }
      else {
        bigcenter = GPoint( bigcenter.x+SHIFT_BIGM, bigcenter.y);
      }
      outer_pos = radius-OFF_BIGM;
      mfont = s_font_bigm;
      mbox  = SZ_BIGM;
    }
    else {
      outer_pos = radius-OFF_MINS;
      mfont = ( settings.boldmins == 0 ) ? s_font_mins : s_font_mins_bold;
      mbox  = SZ_MINS;
    }

    // Im Exaktmodus traegt der Ring jede einzelne Minute, sonst das
    // 5-Minuten-Raster -- beide Male zwoelf Beschriftungen auf einer vollen
    // Umdrehung, siehe MIN_SLOTS.
    for( int i = 0; i < MIN_SLOTS; i++ ) {
      char label[4];
      int  step;   // Drehung des Rings gegenueber der Markierung, in Grad
      if( settings.exactmins == 1 ) {
        int k = i + EXACT_MIN_FIRST;                       // -5 .. +6 Minuten
        snprintf( label, sizeof(label), "%02d", ((min+k) % 60 + 60) % 60 );
        step = -k*MIN_STEP_DEG + (MIN_STEP_DEG*sec/60);
      }
      else {
        snprintf( label, sizeof(label), "%02d", (60 - 5*i) % 60 );
        step = (i*MIN_STEP_DEG) + (min*MIN_STEP_DEG/5) + (MIN_STEP_DEG*sec/300);
      }

      int deg;
      if( settings.flipscreen == 0 ) {
        deg = ( settings.rotation == 0 ) ? (step+90) : (-step+90);
      }
      else {
        deg = ( settings.rotation == 0 ) ? (180-step+90) : (180+step+90);
      }
      angle = DEG_TO_TRIGANGLE(deg);

      outer.y = (int16_t)(-cos_lookup(angle) * (int32_t)outer_pos / TRIG_MAX_RATIO) + bigcenter.y;
      outer.x = (int16_t)(sin_lookup(angle) * (int32_t)outer_pos / TRIG_MAX_RATIO) + bigcenter.x;
      // draw minutes number
      draw_centered(ctx, label, mfont, 2*mbox, mbox, outer);
    }
  }

  // Seconds
  if( settings.timemode == 2 ) {
    // second numbers
    graphics_context_set_text_color(ctx, settings.secscolor );
    outer_pos = radius-OFF_SECS;
    const char *snums[12] =
    {
       "00", "55", "50", "45", "40", "35", "30", "25", "20", "15", "10", "05"
    };

    for( int i = 0; i < 12; i++ ) {
      if( settings.flipscreen == 0 ) {
        if( settings.rotation == 0 ) {
          angle = DEG_TO_TRIGANGLE((i*30)+(sec*6)+90);
        }
        else {
          angle = DEG_TO_TRIGANGLE((-1*i*30)-(sec*6)+90);
        }
      }
      else {
        if( settings.rotation == 0 ) {
          angle = DEG_TO_TRIGANGLE(180-(i*30)-(sec*6)+90);
        }
        else {
          angle = DEG_TO_TRIGANGLE(180+(i*30)+(sec*6)+90);
        }
      }
      outer.y = (int16_t)(-cos_lookup(angle) * (int32_t)outer_pos / TRIG_MAX_RATIO) + bigcenter.y;
      outer.x = (int16_t)(sin_lookup(angle) * (int32_t)outer_pos / TRIG_MAX_RATIO) + bigcenter.x;
      // draw seconds number
      draw_centered(ctx, snums[i],
                    ( settings.boldsecs == 0 ) ? s_font_secs : s_font_secs_bold,
                    2*SZ_SECS, SZ_SECS, outer);
    }
  }

  // Date in conjunction with Hour (2 styles)
  if( settings.timemode == 3 || settings.timemode == 4 || settings.timemode == 5 || settings.timemode == 6 ) {
    // date
    graphics_context_set_text_color(ctx, settings.datecolor );
    if( settings.timemode == 6 ) {
      outer_pos = radius-OFF_DATE6;
    }
    else {
      outer_pos = radius-OFF_DATE45;
    }
    if( settings.timemode == 3 ) {
      outer_pos = radius-OFF_DATE3;
    }

    char date[5][32];
    if( settings.rotation == 1 ) {
      DatePlusDays(t, -2);
    }
    else {
      DatePlusDays(t, 2);
    }
    for( int i = 0; i<5; i++ ) {
      if( settings.timemode == 3 ) {
        strftime( date[i], sizeof(date[i]), "%a %b %d", t);
      }
      if( settings.timemode == 4 ) {
        strftime( date[i], sizeof(date[i]), "%a %d", t);
      }
      if( settings.timemode == 5 ) {
        strftime( date[i], sizeof(date[i]), "%b %d", t);
      }
      if( settings.timemode == 6 ) {
        strftime( date[i], sizeof(date[i]), "%d", t);
      }
      if( settings.rotation == 1 ) {
        DatePlusDays(t, 1);
      }
      else {
        DatePlusDays(t, -1);
      }
    }

    int start = 0;
    int end   = 5;

    hour = t->tm_hour;

    if( settings.fixeddate == 1 )
    {
      start = 2;
      end   = 3;
      hour  = 0;
    }

    for( int i = start; i < end; i++ ) {
      if( settings.flipscreen == 0 ) {
        if( settings.rotation == 0 ) {
          angle =(i*60)+(hour*60/24)-30;
        }
        else {
          angle = (i*60)-(hour*60/24)-30;
        }
      }
      else {
        if( settings.rotation == 0 ) {
          angle = 30-(i*60)-(hour*60/24);
        }
        else {
          angle = 30-(i*60)+(hour*60/24);
        }
      }
      angle = DEG_TO_TRIGANGLE( angle );
      outer.y = (int16_t)(-cos_lookup(angle) * (int32_t)outer_pos / TRIG_MAX_RATIO) + bigcenter.y;
      outer.x = (int16_t)(sin_lookup(angle) * (int32_t)outer_pos / TRIG_MAX_RATIO) + bigcenter.x;
      // draw date
      if( settings.timemode == 3 ) {
        draw_centered(ctx, date[i],
                      ( settings.bolddate == 0 ) ? s_font_date3 : s_font_date3_bold,
                      W_DATE3, SZ_DATE3, outer);
      }
      if( settings.timemode == 4 || settings.timemode == 5 ) {
        draw_centered(ctx, date[i],
                      ( settings.bolddate == 0 ) ? s_font_date45 : s_font_date45_bold,
                      W_DATE45, SZ_DATE45, outer);
      }
      if( settings.timemode == 6 ) {
        draw_centered(ctx, date[i],
                      ( settings.bolddate == 0 ) ? s_font_date6 : s_font_date6_bold,
                      2*SZ_DATE6, SZ_DATE6, outer);
      }
    }
  }
}

static void dot_update_proc(Layer *layer, GContext *ctx) {

  GRect bounds  = layer_get_bounds(s_dot_layer);
  GPoint center = GPoint( bounds.size.w/2, bounds.size.h/2);
  int radius = ARC_OFFSET + (bounds.size.w/4);


  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_color(ctx, settings.linecolor );

  // Laenge des Zeigerstrichs, vom Rand aus gemessen.
  int len;

#ifdef BIG_SCREEN
  // Auf den grossen Displays aus dem Radius der innersten Zahlenreihe
  // ableiten: die festen Laengen der kleinen Modelle liessen hier eine immer
  // groessere Luecke zwischen Strich und Zahl stehen.
  int ring = 0;
  int gap  = GAP_HOUR;
  int side = 0;
  switch( settings.timemode ) {
    case 1: ring = OFF_MINS;   gap = GAP_MINS;   break;
    case 2: ring = OFF_SECS;   gap = GAP_SECS;   break;
    case 3: ring = OFF_DATE3;  gap = GAP_DATE3;  break;
    case 4:
    case 5: ring = OFF_DATE45; gap = GAP_DATE45; break;
    case 6: ring = OFF_DATE6;  gap = GAP_DATE6;  break;
    case 7: ring = OFF_BIGM;   gap = GAP_BIGM;   side = SHIFT_BIGM; break;
    default: break;
  }
  len = (center.x - ARC_OFFSET - side) + (radius - ring) - gap;
#else
  switch( settings.timemode ) {
    case 1:  len = radius - 70;  break;
    case 2:  len = radius - 90;  break;
    case 3:  len = radius - 100; break;
    case 4:
    case 5:
    case 6:
    case 7:  len = radius - 90;  break;
    default: len = radius - 34;  break;
  }
#endif

  if( settings.flipscreen == 0 ) {
    graphics_draw_line(ctx, GPoint( bounds.origin.x, center.y ), GPoint( bounds.origin.x+len, center.y));
  }
  else {
    graphics_draw_line(ctx, GPoint( bounds.size.w, center.y ), GPoint( bounds.size.w-len, center.y));
  }

  if( settings.flipscreen == 0 ) {
    graphics_draw_line(ctx, GPoint( bounds.size.w, center.y ), GPoint( bounds.size.w-STUB_LEN , center.y));
  }
  else {
    graphics_draw_line(ctx, GPoint( bounds.origin.x, center.y ), GPoint( bounds.origin.x+STUB_LEN , center.y));
  }

}


static void app_focus_changing(bool focusing) {
	if (focusing) {
	   layer_set_hidden(window_get_root_layer(s_window), true);
	}
}

static void app_focus_changed(bool focused) {
  if (focused) {
    layer_set_hidden(window_get_root_layer(s_window), false);
    layer_mark_dirty(window_get_root_layer(s_window));
  }
}

static void window_load(Window *window) {

  load_settings();
  load_fonts();

  window_set_background_color(s_window, settings.backcolor);

  GRect bounds = layer_get_bounds(window_get_root_layer(s_window));
  s_screen_w = bounds.size.w;

  s_dot_layer = layer_create(bounds);
  layer_set_update_proc(s_dot_layer, dot_update_proc);
  layer_add_child(window_get_root_layer(s_window), s_dot_layer);

  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_get_root_layer(s_window), s_hands_layer);

  app_focus_service_subscribe_handlers((AppFocusHandlers){
	  .did_focus = app_focus_changed,
	  .will_focus = app_focus_changing
	});

  // Sekundenmodus gleich beim Start beruecksichtigen, sonst laeuft die Uhr
  // bis zur ersten Konfigurationsnachricht im Minutentakt.
  if( settings.timemode == 2 ) {
    tick_timer_service_subscribe(SECOND_UNIT, handle_time_tick );
  }
  else {
    tick_timer_service_subscribe(MINUTE_UNIT, handle_time_tick );
  }
}

static void window_unload(Window *window) {
  unload_fonts();
}

static void init() {
  s_window = window_create();

  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  //Register AppMessage events
  app_message_register_inbox_received(in_received_handler);
  //Largest possible input and output buffer sizes
  app_message_open(256,256);

  window_stack_push(s_window, true);
}

static void deinit() {
  save_settings();
  app_focus_service_unsubscribe();
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
