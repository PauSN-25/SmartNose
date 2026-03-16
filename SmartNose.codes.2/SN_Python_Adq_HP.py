import serial
import matplotlib.pyplot as plt
import csv
from datetime import datetime
import time
import platform
import os
import re

# ---------------- CONFIGURACIÓN ----------------
PUERTO = "COM14"       # Ajusta a tu puerto
BAUDIOS = 115200
TIMEOUT = 0.1
# ------------------------------------------------

# Forzar carpeta del script como working dir
RUTA_BASE = os.path.dirname(os.path.abspath(__file__))
os.chdir(RUTA_BASE)

# --- FUNCIONES AUXILIARES (COMUNICACIÓN) ---
def beep(n=1):
    try:
        if platform.system() == "Windows":
            import winsound
            for _ in range(n):
                winsound.Beep(1000, 500)
                time.sleep(0.1)
        else:
            for _ in range(n):
                print("\a")
                time.sleep(0.1)
    except Exception:
        pass

def teclado_stop(ser):
    try:
        import msvcrt
        if msvcrt.kbhit():
            key = msvcrt.getch().decode().lower()
            if key == 's':
                ser.write(b"STOP\n")
                print("⛔ STOP")
                return True
    except ImportError:
        pass
    return False

def esperar_ready(ser):
    print("🔄 Esperando conexión con Arduino ...")
    while True:
        linea = ser.readline().decode(errors="ignore").strip()
        if linea:
            if linea == "READY":
                print("✅ Arduino listo.")
                break

def esperar_conf(ser):
    while True:
        linea = ser.readline().decode(errors="ignore").strip()
        if linea == "CONF":
            print("✅ Arduino ha recibido configuración inicial.")
            break

# --- GESTIÓN DE ARCHIVOS ---
def limpiar_nombre(texto):
    return re.sub(r'[\\/*?:"<>|]', "", str(texto)).strip().replace(" ", "_")

def preparar_archivo_csv(nombre_carpeta, nombre_ensayo, nombre_muestra):
    # Creamos la carpeta principal (o la usamos si ya existe)
    carpeta_destino = os.path.join(RUTA_BASE, nombre_carpeta)
    os.makedirs(carpeta_destino, exist_ok=True)
    
    fecha_obj = datetime.now()
    timestamp = fecha_obj.strftime("%Y.%m.%d_%H.%M.%S")
    
    # El archivo  incluye Timestamp + Ensayo + Muestra
    nombre_archivo = f"{timestamp}_{nombre_ensayo}_{nombre_muestra}.csv"
    ruta_completa = os.path.join(carpeta_destino, nombre_archivo)
    
    return ruta_completa, timestamp

def parse_float_nan(val_str):
    val_str = val_str.strip()
    if val_str == "NAN" or not val_str:
        return ""
    try:
        return float(val_str)
    except ValueError:
        return ""

# --- ÚNICA INTERFAZ VISUAL: EL RELOJ CIRCULAR CON COLORES DINÁMICOS ---
def actualizar_grafico_progreso(tiempo_transcurrido, tiempo_total, fase_nombre):
    if tiempo_total <= 0:
        return  
        
    if not hasattr(actualizar_grafico_progreso, "ultimo_refresco"):
        actualizar_grafico_progreso.ultimo_refresco = 0
        
    tiempo_actual = time.time()
    
    if (tiempo_actual - actualizar_grafico_progreso.ultimo_refresco < 0.5) and (tiempo_transcurrido < tiempo_total):
        return  
        
    actualizar_grafico_progreso.ultimo_refresco = tiempo_actual
    
    porcentaje_transcurrido = min((tiempo_transcurrido / tiempo_total) * 100, 100)
    porcentaje_restante = max(100 - porcentaje_transcurrido, 0)
    
    sizes = [porcentaje_transcurrido, porcentaje_restante]
    labels = [f'Transcurrido:\n{tiempo_transcurrido:.1f}s', f'Restante:\n{tiempo_total - tiempo_transcurrido:.1f}s']
    
    if fase_nombre.upper() == "VENTILACION":
        color_activo = '#2196F3'  
    elif fase_nombre.upper() == "MEDICION":
        color_activo = '#FFC107'  
    else:
        color_activo = '#4CAF50'  
        
    colors = [color_activo, '#E0E0E0']  
    
    plt.figure(1)  
    plt.clf()  
    plt.pie(sizes, labels=labels, colors=colors, autopct='%1.1f%%', startangle=90, counterclock=False, 
            wedgeprops={'edgecolor': 'white', 'linewidth': 1})
    
    plt.title(f'Fase: {fase_nombre.upper()}\nDuración Total: {tiempo_total:.0f}s', fontsize=12)
    plt.axis('equal')  
    plt.pause(0.001)  

# --- LÓGICA PRINCIPAL ---
def ejecutar_ensayo(ser):
    print("\n" + "="*40)
    print("   NUEVO ENSAYO DE ADQUISICIÓN")
    print("="*40)
    
    # 1. Solicitar nombre de la CARPETA
    nombre_carpeta = input("1. Nombre de la Carpeta: ").strip()
    if not nombre_carpeta: nombre_carpeta = "Datos_Generales"
    nombre_carpeta = limpiar_nombre(nombre_carpeta).upper()
    
    # 2. Solicitar nombre del ENSAYO
    nombre_ensayo = input("2. Nombre del Ensayo: ").strip()
    if not nombre_ensayo: nombre_ensayo = "Ensayo_General"
    nombre_ensayo = limpiar_nombre(nombre_ensayo).upper()
    
    # 3. Solicitar USUARIO
    usuario = input("3. Usuario / Investigador: ").strip()
    if not usuario: usuario = "Anonimo"
    usuario = limpiar_nombre(usuario).upper()
    
    print("\n--- Configuración FASE 0: VENTILACIÓN ---")
    try:
        dur_vent = float(input("   Duración Ventilación (minutos): "))
    except ValueError:
        dur_vent = 1.0; 
        
    print("\n--- Configuración FASE 1: MEDICIÓN ---")
    nombre_muestra = input("   Nombre de la Muestra: ").strip()
    if not nombre_muestra: nombre_muestra = "Muestra_X"
    nombre_muestra = limpiar_nombre(nombre_muestra).upper()
    
    try:
        dur_med = float(input("   Duración Medición (minutos): "))
    except ValueError:
        dur_med = 10.0; 

    lista_fases = [
        {
            "nombre": "VENTILACION", "tipo_muestra": "AIRE_LIMPIO", "camara_estado": "ABIERTA",
            "duracion": dur_vent, "ventilador": 100, "vent_str": "ON"
        },
        {
            "nombre": "MEDICION", "tipo_muestra": nombre_muestra, "camara_estado": "CERRADA",
            "duracion": dur_med, "ventilador": 0, "vent_str": "OFF"
        }
    ]

    # Pasamos los 3 parámetros para crear la ruta
    ruta_csv, timestamp_str = preparar_archivo_csv(nombre_carpeta, nombre_ensayo, nombre_muestra)
    ahora = datetime.now()
    
    columnas = [
        "Fase", "Tiempo_Global_s", "PWM",
        "Temp_A", "Temp_B", "Temp_C",
        "Hum_A", "Hum_B", "Hum_C",
        "Pres_A", "Pres_B", "Pres_C"
    ]
    for i in range(1, 11): columnas.append(f"SA{i}")
    for i in range(1, 11): columnas.append(f"SB{i}")
    for i in range(1, 11): columnas.append(f"SC{i}")

    # Metadata 
    header_metadata = [
        "# --- METADATA ENSAYO ---",
        f"# Carpeta: {nombre_carpeta}",
        f"# Nombre Ensayo: {nombre_ensayo}",
        f"# Fecha y Hora de creacion archivo: {ahora.strftime('%Y-%m-%d %H:%M:%S')}",
        f"# Usuario: {usuario}",
        "# --- CONFIGURACION FASE 0 (VENTILACION) ---",
        f"# Duracion (min): {dur_vent}",
        f"# Ventilador: {lista_fases[0]['vent_str']} | Camara: {lista_fases[0]['camara_estado']}",
        "# --- CONFIGURACION FASE 1 (MEDICION) ---",
        f"# Muestra: {nombre_muestra}",
        f"# Duracion (min): {dur_med}",
        f"# Ventilador: {lista_fases[1]['vent_str']} | Camara: {lista_fases[1]['camara_estado']}",
        "# -----------------------"
    ]
    
    with open(ruta_csv, mode='w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        for linea_meta in header_metadata:
            f.write(linea_meta + "\n")
        writer.writerow(columnas)

    plt.ion()
    tiempo_acumulado_base = 0

    for idx, fase in enumerate(lista_fases):
        print(f"\n🚀 INICIANDO FASE: {fase['nombre'].upper()}")
        
        # Le enviamos al ESP32 el nombre del ensayo 
        ser.write(f"{nombre_ensayo}\n".encode())
        esperar_conf(ser) 

        ser.write(f"{fase['tipo_muestra']}\n".encode()); time.sleep(0.05)
        ser.write(f"{fase['camara_estado']}\n".encode()); time.sleep(0.05)
        ser.write(f"{int(fase['duracion']*60)}\n".encode()); time.sleep(0.05)
        ser.write(f"{fase['ventilador']}\n".encode()); time.sleep(0.05)

        if idx == 0:
            print("🔴 FASE VENTILACIÓN: Asegúrate de que la CÁMARA y el OBTURADOR ESTÉN ABIERTOS.")
        else:
            print("\n" + "!"*50)
            print(f"🛑 ¡ATENCIÓN! CIERRE EL OBTURADOR, INTRODUZCA MUESTRA '{nombre_muestra}' Y CIERRE LA CÁMARA.")
            print("!"*50)
            beep(1)
        
        input(f"👉 Presiona ENTER para arrancar fase {fase['nombre']}... ")
        
        hora_arranque = datetime.now().strftime('%H:%M:%S')
        ser.write(b"ENTER\n") 
        
        print(f"⏳ Midiendo... (Iniciado a las {hora_arranque}) - Pulsa 'S' para parar emergencia")
        fase_activa = True
        tiempo_total_fase_segundos = fase['duracion'] * 60
        
        if hasattr(actualizar_grafico_progreso, "ultimo_refresco"):
            actualizar_grafico_progreso.ultimo_refresco = 0
            
        actualizar_grafico_progreso(0, tiempo_total_fase_segundos, fase['nombre'])
        
        with open(ruta_csv, mode='a', newline='', encoding='utf-8') as f_append:
            f_append.write(f"# --- INICIO REAL FASE {fase['nombre']}: {hora_arranque} ---\n")
            writer = csv.writer(f_append)
            
            while fase_activa:
                if teclado_stop(ser):
                    fase_activa = False
                    break

                try:
                    linea = ser.readline().decode(errors="ignore").strip()
                except:
                    continue
                
                if not linea: continue

                if linea.startswith("DATA"):
                    campos = linea.split("|")
                    
                    try:
                        t_local = float(campos[1])
                        t_global = t_local + tiempo_acumulado_base
                        
                        try:
                            pwm_val = int(campos[-1])
                        except ValueError:
                            pwm_val = 0
                            
                        valores_str = campos[2:-1] 
                        valores_float = list(map(parse_float_nan, valores_str))
                        
                        while len(valores_float) < 39:
                            valores_float.append("")
                        valores_float = valores_float[:39]
                        
                        fila_csv = [fase['nombre'], t_global, pwm_val] + valores_float
                        writer.writerow(fila_csv)
                        f_append.flush()  
                        
                        actualizar_grafico_progreso(t_local, tiempo_total_fase_segundos, fase['nombre'])
                            
                    except Exception as e:
                        print(f"⚠️ Error procesando datos: {e} | Trama recibida: {linea}")

                elif linea == "FIN":
                    print(f"✅ Fase {fase['nombre']} terminada por tiempo.")
                    actualizar_grafico_progreso(tiempo_total_fase_segundos, tiempo_total_fase_segundos, fase['nombre']) 
                    fase_activa = False
                    try: tiempo_acumulado_base += tiempo_total_fase_segundos 
                    except: pass
                
                elif linea == "STOPPED":
                    print(f"⚠️ Fase {fase['nombre']} detenida por comando.")
                    fase_activa = False

    print("\n✅ ENSAYO COMPLETO.")
    beep(3)
    print(f"💾 Datos guardados en: {ruta_csv}")
    
    plt.ioff()
    plt.close()

def main():
    ser = None
    while True:
        try:
            if ser is None or not ser.is_open:
                print(f"🔌 Conectando a {PUERTO}...")
                ser = serial.Serial(PUERTO, BAUDIOS, timeout=TIMEOUT)
                time.sleep(2)
                ser.reset_input_buffer()
                esperar_ready(ser)
            
            ejecutar_ensayo(ser)
            
        except serial.SerialException as e:
            print(f"❌ Error de conexión: {e}")
            print("Reconectando en 5 segundos...")
            if ser:
                try: ser.close()
                except: pass
            ser = None
            time.sleep(5)
        except KeyboardInterrupt:
            print("\n👋 Programa terminado por usuario.")
            if ser: ser.close()
            break
        except Exception as e:
            print(f"❌ Error inesperado: {e}")
            time.sleep(5)

if __name__ == "__main__":
    main()