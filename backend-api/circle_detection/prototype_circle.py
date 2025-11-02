import cv2
import numpy as np

# Bild laden
img = cv2.imread("./backend-api/circle_detection/images/Bild.jpg")

gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
gray = cv2.medianBlur(gray, 5)

# Kreise erkennen (Hough-Transformation)
circles = cv2.HoughCircles(
    gray,
    cv2.HOUGH_GRADIENT,
    dp=1.2,  # Auflösungsverhältnis
    minDist=50,  # minimaler Abstand zwischen Kreisen
    param1=85,  # Canny-Edge-Parameter
    param2=85,  # Empfindlichkeit: kleiner -> mehr Kreise
    minRadius=5,
    maxRadius=500,
)

if circles is not None:
    circles = np.uint16(np.around(circles[0, :]))
    results = []

    for x, y, r in circles:
        # Kreisregion extrahieren
        mask = np.zeros_like(gray)
        cv2.circle(mask, (x, y), r, 255, -1)
        mean_inside = cv2.mean(gray, mask=mask)[0]

        # Ringmaske (Rand)
        ring = np.zeros_like(gray)
        cv2.circle(ring, (x, y), r, 255, 2)
        mean_edge = cv2.mean(gray, mask=ring)[0]

        # Entscheidung: gefüllt oder nicht?
        filled = abs(mean_inside - mean_edge) < 20  # Schwellwert anpassbar
        fill_state = "gefüllt" if filled else "ungefüllt"

        results.append(
            {"x": int(x), "y": int(y), "radius": int(r), "status": fill_state}
        )

        # Visualisierung
        color = (0, 255, 0) if filled else (0, 0, 255)
        cv2.circle(img, (x, y), r, color, 2)
        cv2.circle(img, (x, y), 2, (255, 0, 0), 3)

    print("Erkannte Kreise:")
    for res in results:
        print(res)

    # Originalbild und das neue Bild speichern
    cv2.imwrite("./backend-api/circle_detection/images/Bild_ergebnis.jpg", img)
    # cv2.imshow("Ergebnisse", img)
    # cv2.waitKey(0)
    # cv2.destroyAllWindows()

else:
    print("Keine Kreise gefunden.")
