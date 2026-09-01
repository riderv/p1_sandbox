	// DRAW 3D CUBES
	
    // Настраиваем 3D камеру
    Camera3D camera = { 0 };
    camera.position = Vector3{ 0.0f, 15.0f, 20.0f }; // Позиция камеры
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };     // Куда смотрит камера
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };         // Вектор "верх" камеры
    camera.fovy = 20.0f;                             // Угол обзора
    camera.projection = CAMERA_PERSPECTIVE;          // Перспектива

    // Параметры 3D кубов
    float cubeSize = 0.988f;   // Размер куба
    float spacing = 1.0f;    // Шаг сетки (размер куба + отступ)
    float offset = 4.5f;     // Смещение для центрирования сетки (9 шагов / 2)
    BeginMode3D(camera);

    // Двойной цикл для отрисовки сетки 10х10 в плоскости XZ
    for (int x = 0; x < 10; x++) {
        for (int z = 0; z < 10; z++) {
            // Вычисляем координаты в 3D пространстве
            float posX = x * spacing - offset;
            float posZ = z * spacing - offset;

            // Рисуем заполненный куб
            DrawCube(Vector3{ posX, 0.0f, posZ }, cubeSize, cubeSize, cubeSize, LIME);
            // Рисуем черные грани куба
            DrawCubeWires(Vector3{ posX, 0.0f, posZ }, cubeSize, cubeSize, cubeSize, DARKGREEN);
        }
    }
    EndMode3D();

