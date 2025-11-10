# SmartNose  
### Nariz Electrónica para la Estimación de la Calidad del Aceite de Oliva Virgen (Metodologías de Machine Learning)

Este repositorio alberga el proyecto de investigación **SmartNose**, un prototipo de nariz electrónica diseñado para la **estimación de la calidad del Aceite de Oliva Virgen (AOVE)** y la **detección del estado de las aceitunas** mediante la medición de gases volátiles.

El sistema utiliza sensores **BME688** y metodologías de **Machine Learning (ML)** para el análisis.  
Sus potenciales aplicaciones incluyen la detección de **contaminaciones químicas** en muestras de suelo u otros productos.

---

## 1. Guía de Configuración Completa


Toda la documentación detallada para la instalación, montaje y uso del prototipo se encuentra en la **Wiki** del repositorio.

**[Comienza aquí](https://github.com/PauSN-25/SmartNose/wiki)**

La Wiki contiene todos los pasos necesarios para la puesta en marcha del software y hardware:

- **Instalación del software:** Guía para instalar el **Arduino IDE** (con soporte para ESP32 y las librerías necesarias) y **Python** (incluyendo las dependencias mediante `pip`).  
- **Montaje del hardware:** Lista de materiales, diagrama de conexión y tabla de pines.  
- **Flujo de comunicación:** Explicación detallada de cómo interactúan el firmware del **ESP32** (`SN.ino`) y el script de **Python** (`SmartNose.py`).

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
  primero con papel, luego con jabón y agua, y finalmente con alcohol (solo cuando la SmartNose no esté en uso).  
- **Plástico:** Evita reutilizar platos Petri de plástico muchas veces, ya que pueden retener compuestos y alterar futuras mediciones.  
- **Dosificación y volumen:** Usa una **jeringa graduada en mililitros (ml)** para controlar el volumen de la muestra. Límpiala adecuadamente si se va a reutilizar.  
- **Aumento de superficie:** Se recomienda usar **dos platos Petri de 50 mm** con **5 ml en cada uno** (10 ml total) para aumentar la superficie de emisión de gases.  
- **Aceitunas:** Si se ensayan aceitunas enteras, pueden colocarse directamente en la bandeja de muestras o en platos Petri.  
- **Remoción:** Remueve ligeramente las muestras de aceite justo antes de iniciar la medición para facilitar la emisión inicial de gases.

---

### 3.2 Consejos para la Medición

- **Inicialización (30 minutos):** Antes de cualquier ensayo, inicializa los sensores durante **30 minutos** con la cámara abierta y el ventilador encendido.  
  Esta fase proporciona datos de referencia de las condiciones ambientales.  
- **Fase de ventilación:** Realiza siempre un ensayo de ventilación entre mediciones con muestras (`Muestra: No`, `Cámara: abierta`, `Ventilador: 100%`).  Esto limpia residuos de gases y restablece la línea base.  
- **Cámara cerrada y ventilador apagado:** Durante la medición, asegúrate de que la cámara esté cerrada y el ventilador apagado (`Velocidad: 0%`).  El movimiento de aire puede alterar los resultados.  
- **Tiempo de saturación:** Se recomienda un tiempo de medición entre **10 y 30 minutos** para permitir la reacción completa de los sensores a los gases emitidos.

---

## Estructura del Repositorio

| Carpeta / Archivo       | Descripción |
|--------------------------|-------------|
| `ACOND/`                 | Contiene el código de acondicionamiento de sensores (`ACOND.ino`). |
| `SmartNose.codes.1/`     | Incluye el firmware principal (`SN.ino`) y el script de Python (`SmartNose.py`) correspondientes a la versión 1 del proyecto. |
| `SN.Docs/`               | Carpeta que contiene los siguientes archivos: |
| ├── `SN.Slides.pdf`      | Diapositivas con la explicación del diseño del prototipo y los resultados obtenidos. |
| ├── `BME688.Datasheet.pdf` | Documento técnico del sensor BME688. |
| ├── `ACEITES.PERÚ.xlsx`  | Archivo Excel con los datos brutos (*raw data*) y resumen de los ensayos con aceites de Perú y el análisis de separabilidad de las muestras. |
| `LICENSE`                | Contrato de licencia para el uso del código (Licencia MIT). |
| `README.md`              | Documento de introducción y guía principal del repositorio. |

---

## Autor y Contacto

**Desarrollado por:** Paula Molina Gómez  
**Versión actual:** v1.0.1  
**Fecha de actualización:** 2025-11-05  

**Contacto para soporte o dudas:**  
**paula.molinagomez01@gmail.com**

---

## Licencia

Este proyecto está distribuido bajo la **Licencia MIT**.  
Consulta el archivo [`LICENSE`](https://github.com/PauSN-25/SmartNose/blob/main/LICENSE) para más detalles.
