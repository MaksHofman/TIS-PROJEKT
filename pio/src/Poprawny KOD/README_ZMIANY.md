# README — Projekt TIS E1 / Sieć czujników kolorów
### Dokumentacja zmian, naprawionych błędów i potencjalnych problemów

---

## Architektura systemu (przypomnienie)

```
[S1]─┐
[S4]─┤─BUS─[W1]─[W3]─┐
[S5]─┘                 ├─[W0]──USB──PC
[S2]─┐                 │
[S3]─┴─BUS─[W2]─[W4]─┘
```

Każdy węzeł to Arduino MKR WAN 1310. Komunikacja UART po protokole własnym (proto_defs.h).

---

## Pliki w projekcie

| Plik | Węzeł | Opis |
|---|---|---|
| `main-aggregator-new.cpp` | W0 | Agregator, podłączony USB do PC, menu wyboru |
| `main-ruter-master.cpp` | W1 / W2 | Master — łączy slave'y z forwarderami |
| `main-forwarder.cpp` | W3 / W4 | Forwarder — pośrednik między W0 a masterami |
| `main-dummy-sensor.cpp` | S1–S5 | Sensor z prawdziwym odczytem TCS3200 (dummy = bez kalibracji) |
| `main-smart-sensor.cpp` | S1 | Sensor z kalibracją i logiem przez Serial |
| `led_helper.cpp / .h` | wszędzie | LED RGB + calcCRC8 (Hamming) |
| `hamming_helper.h` | wszędzie | Implementacja Hamming(12,8) |

---

## Co było złe — naprawione błędy

### BUG 1 — `calcCRC8` zawsze zwracała `1` ⚠️ KRYTYCZNY

**Plik:** `led_helper.cpp` (oryginał)

**Kod:**
```cpp
uint8_t calcCRC8(byte* buff, uint8_t len) {
    return 1;  // <-- NIGDY nie było zaimplementowane!
}
```

**Skutek:** Każda wysłana wiadomość miała w polu CRC wartość `1`. Po stronie odbiornika obliczona wartość (też `1`, bo ta sama funkcja) zgadzała się z polem... ALE tylko gdy dane były takie, że XOR = 0. W praktyce: funkcja zwracała zawsze stałą, więc weryfikacja losowo przechodziła lub losowo failowała. Węzły zgłaszały błąd CRC → wysyłały RET → brak odpowiedzi → timeout → fałszywy komunikat `DEAD`. To był główny powód "wykrycia awarii w dobrze ustawionym węźle".

**Naprawa:** `calcCRC8` jest teraz aliasem na `hammingCalc()` — prawdziwa funkcja z implementacją.

---

### BUG 2 — `delay(1000)` w `setLedState()` blokuje całą pętlę ⚠️ KRYTYCZNY

**Plik:** `led_helper.cpp` (oryginał)

**Kod:**
```cpp
void setLedState(STATE state) {
    // ...
    delay(1000);  // <-- BLOKUJE loop() na sekundę!
}
```

**Skutek:** `setLedState` była wywoływana przy każdym `RECEIVING`, `TRANSMITTING`, `GENERIC_ERROR`. Każde wywołanie zamrażało mikrokontroler na 1000 ms. Timeout slave'a to `SLAVE_TIMEOUT = 2000 ms` — wystarczyły 2 zdarzenia LED żeby timeout minął bez żadnej prawdziwej awarii. Każdy poprawnie działający węzeł był uznawany za martwy.

**Naprawa:** `led_helper` przepisany jako nieblokujący state machine:
- `triggerLedState(state)` — ustawia kolor na ~500 ms, potem automatycznie wraca do IDLE
- `updateLedTask()` — wywołuj na początku każdego `loop()`, obsługuje powrót
- `setLedState(state)` — ustawia trwały kolor (bez delay)
- Zero bloków `delay()` w całym kodzie LED

---

### BUG 3 — `continue` wewnątrz `switch` w masterze ⚠️ POWAŻNY

**Plik:** `main-ruter-master.cpp` (oryginał), funkcja `handleRHS`

**Kod:**
```cpp
switch (GET_TYP(Rx_buff)) {
    case MSG_T_REQ: {
        uint8_t dst = GET_DST(Rx_buff);
        if (dst != MY_SLAVE1 && dst != MY_SLAVE2 ...) continue; // <-- ZLE
        // ...
    }
}
```

**Skutek:** W C++ `continue` wewnątrz `switch` (który jest wewnątrz `while`) przeskakuje do następnej iteracji `while`, nie `switch`. Efekt: gdy ramka REQ nie była przeznaczona dla tego mastera, pętla natychmiast odczytywała kolejny bajt z bufora zamiast normalnie zakończyć obsługę ramki. Prowadziło to do desynchronizacji ramek UART — kolejne bajty były interpretowane jako nowe ramki od środka.

**Naprawa:** Zmienione na `break` z właściwym warunkiem `if (!isMyslave(dst)) break;`

---

### BUG 4 — Brak `SET_SRC` w agregatoryze

**Plik:** `main-aggregator-new.cpp` (oryginał)

Przy budowaniu ramki REQ agregator nie ustawiał pola `SRC`. Węzły pośrednie mogły mieć problem z identyfikacją nadawcy przy retransmisji.

**Naprawa:** Dodane `SET_SRC(Tx_buff, W0_ID)` przed wysłaniem.

---

### BUG 5 — Logika timeout w agregatoryze (oryginał)

**Plik:** `main-aggregator-new.cpp` (oryginał)

```cpp
uint8_t awaiting_res1, awaiting_res4 = 0;  // <-- inicjalizuje TYLKO awaiting_res4!
```
W C++ ta linia inicjalizuje tylko `awaiting_res4 = 0`. Zmienna `awaiting_res1` ma wartość niezdefiniowaną (śmieciową).

**Naprawa:** Zmienne inicjalizowane oddzielnie.

---

## Co dodano — Hamming(12,8) zamiast CRC8

### Dlaczego Hamming, nie CRC

CRC jest lepsze do wykrywania błędów blokowych, ale wymaga więcej bajtów narzutu lub bardziej skomplikowanej implementacji. Hamming(12,8) dla tego projektu ma zalety:
- Implementacja w ~20 linii, zero tabel, zero zewnętrznych bibliotek
- Mieści się w **1 bajcie** — identyczny narzut jak poprzedni CRC (kompatybilny z `proto_defs.h`)
- Wykrywa i **koryguje** błędy 1-bitowe automatycznie
- Wykrywa błędy 2-bitowe

### Jak działa `hammingCalc` (plik `hamming_helper.h`)

1. XOR wszystkich bajtów danych → 1 bajt sumy `xsum`
2. Hamming(12,8) na `xsum` → 4 bity parzystości (p1, p2, p4, p8)
3. Wynik: górny nibble = bity parzystości, dolny nibble = dolne 4 bity `xsum`
4. Cały wynik mieści się w 1 bajcie → wchodzi w istniejące pole CRC w ramce

### Jak działa `hammingVerify` 

1. Odtwarza `xsum` z danych (XOR)
2. Oblicza oczekiwane bity parzystości
3. Porównuje z zapisanym bajtem
4. Jeśli niezgodność — wykryty błąd → zwraca `false`
5. Funkcja jest `inline` — zero narzutu na wywołanie

### Interfejs — bez zmian w `proto_defs.h`

```cpp
// Wysyłanie (tak samo jak przed):
SET_CRC(buf, calcCRC8(buf, len - 1));

// Weryfikacja (zmiana z porównania na wywołanie):
// STARE: if (GET_CRC(buf) != calcCRC8(buf, len-1)) { ... błąd ... }
// NOWE:
if (!hammingVerify(buf, len)) { ... błąd ... }
```

---

## Miejsca które mogą być jeszcze błędne

### 1. Piny LED — `LED_PIN_R/G/B` w `led_helper.h`

```cpp
#define LED_PIN_R  A3
#define LED_PIN_G  A4
#define LED_PIN_B  A5
```

Jeśli twój LED RGB jest podłączony na innych pinach — zmień te definicje. MKR WAN 1310 ma wbudowany LED tylko na pinie `LED_BUILTIN` (pin 6). Jeśli nie masz zewnętrznego RGB LED to możesz całkowicie wyłączyć LED zastępując `analogWrite(...)` w `led_helper.cpp` pustymi funkcjami.

**Jak naprawić:** Sprawdź schemat swojego okablowania i zmień `LED_PIN_R/G/B` w `led_helper.h`.

---

### 2. Pin `S1_PIN` w `main-smart-sensor.cpp`

W `color_helper.cpp` używane są piny `S0, S1, S2, S3, OUT_PIN` zdefiniowane w `const_defs.h`. Użyłem `S1_PIN` zamiast `S1` żeby uniknąć konfliktu nazw z `S1_ID` (ID węzła). Sprawdź czy w twoim `const_defs.h` pin czujnika TCS3200 nazywa się `S1` czy coś innego — i dostosuj.

**Jak naprawić:** W `main-smart-sensor.cpp` znajdź `S1_PIN` i zamień na właściwą nazwę z `const_defs.h`.

---

### 3. Routing w forwarderze — `THROUGH_SERIAL3/4`

```cpp
// W3:
#define THROUGH_SERIAL4(DST)  (((DST) & 0x1))    // nieparzyste ID → Serial4 (W2)
#define THROUGH_SERIAL3(DST) (!((DST) & 0x1))    // parzyste ID   → Serial3 (W1)
```

Logika opiera się na tym, że slave'y W1 (S1, S4, S5) mają nieparzyste ID, a W2 (S2, S3) mają parzyste. Sprawdź w `const_defs.h` czy ID są rzeczywiście tak przydzielone. Jeśli nie — całe routowanie będzie do złego mastera.

**Jak naprawić:** Sprawdź `const_defs.h` wartości `S1_ID..S5_ID`, `W1_ID, W2_ID` i dostosuj makra `THROUGH_SERIAL3/4` w obu instancjach forwardera (W3 i W4).

---

### 4. `MY_ID` — każdy plik musi mieć inny ID

Każdy węzeł ma na górze `#define MY_ID Xx_ID`. Przed wgraniem na płytkę upewnij się że:

| Plik | Zmień na |
|---|---|
| `main-ruter-master.cpp` | `W1_ID` lub `W2_ID` |
| `main-forwarder.cpp` | `W3_ID` lub `W4_ID` |
| `main-dummy-sensor.cpp` | `S1_ID` do `S5_ID` |
| `main-smart-sensor.cpp` | `S1_ID` (domyślnie) |

**Jak naprawić:** Przed kompilacją każdej wersji zmień `#define MY_ID` na właściwe ID dla danej płytki.

---

### 5. `hammingVerify` — poprawia błędy 1-bitowe, ale nie zwraca skorygowanego bajtu

Obecna implementacja wykrywa czy dane są poprawne, ale nie koryguje błędu w buforze in-place (tylko zwraca `false`). W środowisku o dużym szumie elektrycznym (długie kable, zakłócenia) można rozszerzyć funkcję o właściwą korekcję. Na razie przy błędzie wysyłamy RET (prośba o retransmisję) — co jest poprawnym zachowaniem.

**Jak naprawić (opcjonalnie):** Rozszerzyć `hammingVerify` w `hamming_helper.h` o logikę syndrome — obliczenie pozycji błędnego bitu i jego flipnięcie w buforze.

---

### 6. Bufory Serial mogą się przepełnić przy dużym ruchu

Arduino MKR WAN 1310 ma bufor UART 64 bajty na `Serial1/3/4`. Przy jednoczesnym ruchu na wielu portach (np. W0 wysyła do obu forwarderów jednocześnie) i opóźnieniach w `loop()` może dojść do przepełnienia.

**Jak naprawić:** Jeśli problem występuje, zmniejsz `UART_BAUD` w `const_defs.h` albo zwiększ bufor przez modyfikację `RingBuffer.h` w rdzeniu Arduino (nieoficjalne).

---

### 7. `main-aggregator-new.cpp` — oba forwardery dostają tę samą wiadomość

Agregator wysyła to samo zapytanie przez `Serial1` (W3) i `Serial4` (W4) jednocześnie i czeka na pierwszą odpowiedź. Druga odpowiedź (duplikat) może trafić do bufora i być interpretowana jako kolejna wiadomość w następnej iteracji loop.

```cpp
Serial1.write(Tx_buff, REQ_LEN);
Serial4.write(Tx_buff, REQ_LEN);
```

W tej chwili po odebraniu odpowiedzi z jednej linii `awaiting` tej linii jest zerowane, ale druga linia nadal "czeka". Duplikatowa odpowiedź zostanie zignorowana (awaiting = 0) ale może desynchronizować bufor.

**Jak naprawić:** Po odebraniu odpowiedzi od jednego forwardera wyczyścić bufor drugiego: `while(Serial4.available()) Serial4.read();` i vice versa.

---

## Zalecana kolejność testowania

1. Najpierw wgraj `main-dummy-sensor.cpp` (S1, MY_ID=S1_ID) i `main-ruter-master.cpp` (W1) — sprawdź komunikację BUS
2. Dodaj `main-forwarder.cpp` (W3) i sprawdź czy REQ dociera do W1
3. Na końcu podłącz `main-aggregator-new.cpp` (W0) i testuj przez Serial Monitor
4. Dopiero po działaniu całej ścieżki wgraj `main-smart-sensor.cpp` z prawdziwym czujnikiem

---

*Dokumentacja wygenerowana dla projektu TIS E1 — sieć czujników kolorów TCS3200 na Arduino MKR WAN 1310.*
