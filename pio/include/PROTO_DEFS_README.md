## Ogólny format wiadomości:
```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | dst_id| TYP |    PAYLOAD   (var length)     |      CRC      |0|
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

## TYP (wiem, że starczyłyby 2 bity, ale for future usage dałem 1):
- Request - żądanie odczytu z czujnika
- Response - odpowiedź z odczytem z czujnika
- Dead - dalszy węzeł nie odpowiada. Przesyłana po upłynięci timeout'u. Jakiego? Zdefiniujcie makro, żeby można było metodą prób i błędów

## Wiadomość request (2B):
- dst_id (4 bity): ustawione na id czujnika z którego chcemy odczytać wartość
- typ (3 bity): ustawiony na MSG_T_REQ
- PAYLOAD (0 bitów): brak (CRC ściśnięte)
- bit padding'u 0
- CRC-8

### Format:
```
    0                   1
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | dst_id| TYP |0|      CRC      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

## Wiadomość response (4B):
- dst_id (4 bity): W0_ID
- typ (3 bity): MSG_T_RES
- PAYLOAD (12 bitów): id czujnika, z którego pochodzi odpowiedź (src_id) + odczytana wartość (read)
- 5 bitów padding'u 0
- CRC-8

### FORMAT:
```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | dst_id| TYP | src_id|      read     |    0    |      CRC      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

## Wiadomość dead (3B):
- dst_id (4 bity): W0_ID
- typ (3 bity): MSG_T_DEAD
- PAYLOAD (8 bitów): id węzła, który wykrył awarię (src_id) + id węzła/czujnika, który nie odpowiada (dead_id)
- 1 bit padding'u 0
- CRC-8

### Format:
```
    0                   1                   2
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | dst_id| TYP | src_id|dead_id|0|      CRC      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

## Przesyłanie bufora
- `Serial.write(buf, len)`

## Odczytanie z bufora
- `Serial.available()` - zwraca liczbę bajtów w buforze SERCOM (max 64)
- `Serial.readBytes(buffer, length)`

## Uwaga na synchronizację ramek:
- pozycja CRC-8 jest odczytywana na podstawie typu wiadomości, czyli jeżeli typ był przekłamany to odczyta złe CRC (ok, w sumie i tak wiadomość była zepsuta, ale mogło przekłamać krótszą na dłuższą (w zależności jak zrobicie czytanie to się może zawiesić) lub dłuższą na krótszą (jak nie odczytacie wszystkiego, to w buforze zostaną śmieci i wszystkie wiadomości będą błędne))
- TLDR: Zawsze sprawdzajcie za pomocą `Serial.available()` ile jest do odczytania z bufora i po każdej błędnej wiadomości czyśćcie bufor UART
