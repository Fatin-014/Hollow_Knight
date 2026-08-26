        // =========================
        // CAMERA
        // =========================

        Vector2 target = {
            posx + player.width / 2,
            posy + player.height / 2
        };

        camera.target.x +=
            (target.x - camera.target.x) * 5.0f * dt;

        camera.target.y +=
            (target.y - camera.target.y) * 5.0f * dt;