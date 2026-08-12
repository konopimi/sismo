https://sismoinfo.co

IG:sismoinfo_co
IG:neokpm

vainilla.js

LAS REGLAS SON:
Mínimo uso posible de librerias
vainilla js
Solo 2 archivos importan (monolítico)
server.js
public/index.html
VIBECODE A LO QUE MARCA SIN MIEDO

---

cd public/ &&
npx browser-sync start --server --files "public/index.html, server.js,"

Forks activos:

- https://github.com/konopimi/sismo/issues?q=is%3Apr+author%3Amigueai17bus-svg

# Backlog / Bugs — Para revisión del equipo

## 🔴 Bugs importantes (prioridad alta)

1. **Subida de fotos en celular rota**
   Al intentar subir una foto desde el celular, la app manda directo a la cámara y no permite seleccionar fotos desde la galería.

2. **No se puede cambiar la ubicación después de creado un registro**
   En Mascotas y Personas, una vez creado el item, no hay forma de editar/actualizar su ubicación.

## 🟡 Mejoras funcionales (requieren Pull Request)

3. **Indicador de comentarios en las cards**
   Las cards necesitan un indicador visual de comentarios y mostrar el último comentario recibido.

4. **Contexto en el modal de selección de ubicación en el mapa**
   El modal donde se selecciona la ubicación en el mapa debe mostrar información mínima de contexto del item.

5. **Relacionar personas/mascotas con múltiples ubicaciones**
   Actualmente solo se puede asociar una ubicación por persona/mascota. Debería poder relacionarse con múltiples ubicaciones posibles en el mapa.

6. **Pestaña de colaboradores (voluntarios)**
   Agregar una pestaña dedicada a colaboradores/voluntarios.

7. **Borrador de chat en vivo**
   Implementar un MVP simple de chat en vivo (⚠️ no sobre-ingenierizar), usando **Valtio-Y** y **Hocuspocus**. Debe soportar múltiples canales predefinidos de chat.

8. **Copiar enlace directo desde los modales**
   En los modales de Personas, Mascotas y Ubicaciones, agregar un botón para copiar al portapapeles el link específico del item (con sus query params).

9. **Virtualización ligera para listas**
   Las listas se están volviendo lentas. Se necesita una solución de virtualización minimalista y liviana (sin dependencias pesadas).

10. **Items cercanos en el modal de lugar**
    El modal de lugar muestra el mapa; debería mostrar también los items (personas, mascotas, anuncios) que estén en un radio cercano a esa ubicación.

11. **Selector de ciudad al crear ubicaciones**
    En la pestaña de Ubicaciones, al crear un item se requiere agregar un selector de ciudad.

12. **Opciones de navegación externas para ubicaciones**
    Actualmente los links de ubicación mandan directo a Google Maps. En su lugar, debería mostrarse un selector con las opciones:
    - Ver aquí (dentro de la app)
    - Ver en Google Maps
    - Ver en Waze

13. **Comentarios como text area**
    El campo para crear comentarios debe ser un `<textarea>`, y renderizarse con la etiqueta `<pre></pre>` (igual que se hace en el item de noticias).

## 🟢 Pregunta abierta

- **¿Cuál es la mejora más pequeña con el mayor retorno inmediato que deberíamos priorizar?**
  (Para discutir en equipo antes de repartir los PRs.)
