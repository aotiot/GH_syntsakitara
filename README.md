# 🎸 Guitar Hero Ohjain Syntetisaattori (ESP32 + Mozzi)

Tämä projekti muuttaa tavallisen Guitar Hero -ohjaimen (esim. Wii, PS3, Xbox 360) täysiveriseksi, itsenäiseksi digitaaliseksi syntetisaattoriksi. Järjestelmän sydämenä toimii **ESP32-mikrokontrolleri**, joka hyödyntää voimakasta **Mozzi-audiokirjastoa** tuottaakseen ääntä suoraan korkealaatuiselle **I2S DAC** -moduulille. 

Ohjain tukee sointujen ja yksittäisten nuottien soittamista, neljän oktaavin äänialaa sekä reaaliaikaisia DSP-efektejä analogisen joystickin avulla ohjattuna.

## ✨ Ominaisuudet

* **Huipputason Äänenlaatu:** Ohittaa ESP32:n sisäisen kohinaisen DAC:n käyttämällä ulkoista I2S DAC -moduulia (esim. PCM5102A).
* **Moniytimisyys (Dual-Core):** Painikkeiden luku ja debouncing on eriytetty FreeRTOS:n avulla ytimelle 0 (Core 0), jättäen ytimen 1 (Core 1) täysin omistetuksi keskeytymättömälle audiosynteesille.
* **Nuotti- ja Sointutila:** `Select`-painikkeella voit vaihtaa saumattomasti yksittäisten nuottien ja viiden yleisimmän kitarasoinnun (C, D, Em, Am, G) välillä.
* **Neljän Oktaavin Ääniala:** Joystickin painike (`SW`/`Z`) kiertää neljän eri oktaavialueen läpi.
* **Sisäänrakennetut Efektit:** Joystickin suuntapainikkeilla voit kytkeä päälle/pois neljä eri efektiä:
  * ⬆️ **Distortion** (Särö)
  * ⬇️ **Vibrato** (LFO taajuusmodulaatio)
  * ⬅️ **Tremolo** (LFO äänenvoimakkuuden modulaatio)
  * ➡️ **LPF** (Alipäästösuodin / Muffled tone)
* **Panic/Reset -painike:** `Start`-painike toimii hätäkatkaisimena, joka hiljentää äänen välittömästi ja palauttaa kaikki efektit ja tilat perusasetuksille.

---

## 🛠️ Tarvittavat Komponentit

1. **ESP32 Development Board** (esim. DevKit V1)
2. **I2S DAC -moduuli** (esim. PCM5102A tai UDA1334A)
3. **Guitar Hero -ohjain** (purettuna laitteistohakkerointia varten)
4. **Analoginen Joystick -moduuli** (esim. KY-023)
5. Vahvistin (esim. PAM8403) ja sopiva kaiutin (esim. 4Ω 3W)
6. Virtalähde (esim. 2x 18650 akku rinnan + TP4056-latausmoduuli suojapiirillä)

---

## 🔌 Kytkentäkaavio (Pinout)

Kaikki painikkeet ja kytkimet (Fretit, Strum, Joystick-suunnat) kytketään ESP32:n GPIO-pinnin ja **yhteisen maadoituksen (GND)** väliin. Koodi käyttää ESP32:n sisäisiä ylösvetovastuksia (`INPUT_PULLUP`).

### Guitar Hero Ohjain
| Komponentti | Toiminto | ESP32 GPIO |
| :--- | :--- | :--- |
| **Vihreä Fret** | C4 / C-duuri | `GPIO 32` |
| **Punainen Fret** | D4 / D-duuri | `GPIO 33` |
| **Keltainen Fret**| E4 / E-molli | `GPIO 25` |
| **Sininen Fret** | F4 / A-molli | `GPIO 26` |
| **Oranssi Fret** | G4 / G-duuri | `GPIO 27` |
| **Strum Ylös** | Liipaisin (Note On)| `GPIO 14` |
| **Strum Alas** | Liipaisin (Note On)| `GPIO 23` |
| **Select** | Sointu/Nuotti -tila | `GPIO 15` |
| **Start** | Reset / Panic Button | `GPIO 13` |

### Joystick (Efektit)
| Suunta | Efekti / Toiminto | ESP32 GPIO |
| :--- | :--- | :--- |
| **Ylös** | Särö (Distortion) | `GPIO 4` |
| **Alas** | Vibrato | `GPIO 5` |
| **Vasen** | Tremolo | `GPIO 16` |
| **Oikea** | Alipäästösuodin (LPF)| `GPIO 17` |
| **Painike (SW)** | Oktaavin vaihto | `GPIO 18` |

### I2S DAC (Audio Out)
| DAC Pinni | ESP32 GPIO |
| :--- | :--- |
| **
