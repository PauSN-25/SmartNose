# SmartNose  
### Nariz Electrónica para la Estimación de la Calidad del Aceite de Oliva (Metodologías de Machine Learning)

Este repositorio alberga el proyecto de investigación **SmartNose**, un prototipo de nariz electrónica diseñado para la **estimación de la calidad del Aceite de Oliva** mediante la medición de gases volátiles.

El sistema utiliza sensores **BME688** y metodologías de **Machine Learning** para el análisis.  
Sus aplicaciones potenciales abarcan cualquier caso de investigación que requiera analizar y comparar perfiles volátiles en una amplia variedad de muestras, tanto dentro como fuera del sector oleícola.

---

## 1. Guía de Configuración Completa


Toda la documentación detallada para la instalación, montaje y uso del prototipo se encuentra en la **Wiki** del repositorio.

**[Comienza aquí](https://github.com/PauSN-25/SmartNose/wiki)**

La Wiki contiene todos los pasos necesarios para la puesta en marcha del software y hardware:

- **Instalación del software:** Guía para instalar el **Arduino IDE** (con soporte para ESP32 y las librerías necesarias) y **Python** (incluyendo las dependencias mediante `pip`).  
- **Montaje del hardware:** Lista de materiales, diagrama de conexión y tabla de pines.  
- **Flujo de comunicación:** Explicación detallada de cómo interactúan el firmware del **ESP32** (`SN_HP.ino`) y el script de **Python** (`SN_Python_Adq_HP.py`).

---

## 2. Acondicionamiento y Estabilización de Sensores

Los sensores **BME688**, especialmente los sensores de gas (MOX), requieren un periodo inicial de funcionamiento continuo (conocido como *burn-in* o *acondicionamiento*) cuando son nuevos, para estabilizar sus lecturas.

Para este propósito, se incluye el código `ACOND.ino` en la carpeta **ACOND**:

| Código       | Descripción                                                                 | Requisitos |
|---------------|------------------------------------------------------------------------------|-------------|
| `ACOND.ino`  | Código para la operación continua de los sensores, esencial para el acondicionamiento inicial (mínimo 24 horas). | Instalación completa del Arduino IDE, librerías necesarias y montaje del circuito (ver sección 1. de la Wiki). |

### Instrucciones de uso
1. Asegúrate de que el hardware y el entorno Arduino estén configurados (según la Wiki).  
2. Carga el código `ACOND.ino` en el **ESP32**.  
3. Abre el **Monitor Serie** del Arduino IDE (configurado a 115200 baudios) para verificar la comunicación directa de los datos.  

> Este código admite entre **1 y 3 sensores** conectados simultáneamente.  
> Si se utilizan menos de tres sensores, los valores correspondientes a los sensores no conectados mostrarán **NaN** (*Not a Number*) en el **Monitor Serie**, pero el código continuará funcionando correctamente con los sensores disponibles.

---

## 3. Guía para la Ejecución de Ensayos

Esta sección proporciona recomendaciones para la **preparación y manejo físico de las muestras** durante la experimentación con el prototipo SmartNose.  
Las siguientes pautas se basan en la experimentación previa y pueden actualizarse conforme avance el proyecto.

Se recomienda leer el apartado **4.1: Flujo de Comunicación** en la Wiki para conocer los procedimientos y la información intercambiada por el código.

---

### 3.1 Preparación de Muestras y Cámara

- **Recipientes:** Para aceites, se recomienda usar platos Petri de **50 mm de diámetro**, preferiblemente de cristal (o plástico).  
- **Limpieza:** Los platos Petri utilizados con aceite deben limpiarse rigurosamente:  
  primero con papel, luego con jabón y agua, secar de nuevo con papel y dejarlos al aire para eliminar posibles restos volátiles.
- **Plástico:** Evita reutilizar platos Petri de plástico muchas veces, ya que pueden retener compuestos y alterar futuras mediciones.  
- **Dosificación y volumen:** Usa una **jeringa graduada en mililitros (ml)** para controlar el volumen de la muestra. Límpiala adecuadamente si se va a reutilizar.  
- **Aumento de superficie:** Se recomienda usar **dos platos Petri de 50 mm** con **5 ml en cada uno** (10 ml total) para aumentar la superficie de emisión de gases.  
- **Aceitunas:** Si se ensayan aceitunas enteras, pueden colocarse directamente en la bandeja de muestras o en platos Petri.  
- **Remoción:** Remueve ligeramente las muestras de aceite justo antes de iniciar la medición para facilitar la emisión inicial de gases.

---

### 3.2 Consejos para la Medición

- **Fase de Ventilación (Fase 0 - Automática):** El script integra una fase inicial de purga obligatoria antes de medir cada muestra, configurando el ventilador al 100% por defecto. **Para el primer ensayo del día, se recomienda configurar una duración de 30 minutos** para que los sensores alcancen su temperatura de trabajo y estabilicen su línea base. Para los ensayos consecutivos, bastará con tiempos más cortos para limpiar residuos de gases. Asegúrate físicamente de que **la cámara y el obturador estén abiertos** cuando la consola te lo indique.
- **Fase de Medición (Fase 1 - Automática):** Para la toma de datos, el programa apaga automáticamente el ventilador (0%) para evitar que el flujo de aire altere los resultados. Cuando aparezca el aviso por pantalla, introduce la muestra y asegúrate de **cerrar herméticamente la cámara y los obturadores** antes de presionar ENTER.
- **Tiempo de saturación:** Para la fase de medición, se recomienda configurar una duración de entre **10 y 30 minutos**. Este tiempo permite la acumulación de los gases volátiles emitidos por la muestra en la cámara cerrada, garantizando la reacción completa de los sensores.
---

## Estructura del Repositorio

| Carpeta / Archivo        | Descripción |
|--------------------------|-------------|
| `ACOND/`                 | Contiene el código de acondicionamiento de sensores (`ACOND.ino`). |
| `SmartNose.codes.1/`     | Incluye el firmware principal (`SN.ino`) y el script de Python (`SmartNose.py`) correspondientes a la versión 1 del proyecto. |
| `SmartNose.codes.2/`     | Versión actualizada con configuración de **perfiles térmicos**. Incluye el firmware (`SN_HP.ino`) y dos scripts de Python: <br><br> 🔹 `SN_Python_Adq_HP.py`: Script para la adquisición de datos y configuración de los ensayos con fases automáticas de ventilación y medición. <br> 🔹 `SN_Python_Analisis_HP.py`: Script de procesamiento y análisis masivo de datos. Lee múltiples archivos CSV, realiza control de calidad (filtrando señales anómalas por correlación), normaliza y promedia repeticiones para generar curvas maestras. Extrae un vector de 120 parámetros característicos por muestra (4 parámetros x 30 variables) para realizar análisis de Componentes Principales (PCA), cálculo de distancias euclídeas y evaluación de la importancia/carga de cada variable. |
| `SN.Docs/`               | Carpeta que contiene la documentación técnica y académica del proyecto: |
| ├── `TFG_MEMORIA.pdf`    | Memoria completa del TFG que incluye el desarrollo de hardware y software, resultados experimentales, análisis económico y planos del diseño mecánico. |
| ├── `TFG-DEFENSA.pdf`    | Presentación de la defensa del TFG (sirve como un resumen visual del proyecto). |
| ├── `SN.Slides-06.11.25.pdf` | Diapositivas con la explicación del diseño del prototipo y los resultados obtenidos en fases previas. |
| ├── `BME688.Datasheet.pdf` | Documento técnico oficial del sensor BME688. |
| ├── `ACEITES.PERÚ.xlsx`  | Archivo Excel con los datos brutos (*raw data*) y el análisis de separabilidad de muestras con aceites de Perú. |
| ├── `Informe_Analisis_CE8/` | Carpeta con el informe en Excel del Caso de Estudio 8 (desarrollado en la memoria) e imágenes de los gráficos generados en dicho análisis. |
| `LICENSE`                | Contrato de licencia para el uso del código (Licencia MIT). |
| `README.md`              | Documento de introducción y guía principal del repositorio. |## Autor y Contacto

**Desarrollado por:** Paula Molina Gómez  
**Versión actual:** v1.0.1  
**Fecha de actualización:** 2025-11-05  

**Contacto para soporte o dudas:**  
**paula.molinagomez01@gmail.com**

---

## Licencia

Este proyecto está distribuido bajo la **Licencia MIT**.  
Consulta el archivo [`LICENSE`](https://github.com/PauSN-25/SmartNose/blob/main/LICENSE) para más detalles.
