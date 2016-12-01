timeline = {
    [1] = function()
        -- Create few asteroids
        game.add_asteroid(32.11, 33.1, 20, 3, 3.14)
        game.add_asteroid(-240.11, -400.1, 10, 20, -1.45)
        game.add_asteroid(-180.11, 240.1, 10, -20, 4.45)
        game.add_asteroid(380.11, 340.1, -20, -20, 0.85)

        -- Add an enemy
        game.add_enemy(0, -350, 50)
    end,

    [5] = function()
        game.add_asteroid(230, 400, -20, -30, 2.0)
    end,

    [10] = function()
        game.add_asteroid(-230, -400, 30, 40, -3.14)
    end
}

time = 0

function tick()
    time = time + 1

    local event = timeline[time]
    if event then
        event()
    end
end