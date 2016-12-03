require 'math'

-- Globals
time = 0
start_stage = 0
stage = nil

-- Level stages
level = {
    [0] = function(offset)
        gen_random_asteroids(offset)
        game.add_enemy(0, offset - game.SCREEN_HEIGHT / 2)
    end,

    [1] = function(offset)
        gen_random_asteroids(offset)
        game.add_enemy(-100, offset - 300)
        game.add_enemy(0, offset - 250)
        game.add_enemy(100, offset - 300)
    end,

    [2] = function(offset)
        gen_random_asteroids(offset)
        game.add_enemy(-300, offset + 200)
        game.add_enemy(-200, offset + 200)
        game.add_enemy(300, offset + 100)
        game.add_enemy(200, offset + 100)
    end,

    [3] = function(offset)
        gen_random_asteroids(offset)
        game.add_enemy(-50, offset + 50)
        game.add_enemy(50, offset - 50)
        game.add_enemy(-350, offset + 250)
        game.add_enemy(-250, offset + 150)
        game.add_enemy(350, offset - 250)
        game.add_enemy(250, offset - 150)
    end,

    [4] = function(offset)
        gen_random_asteroids(offset)
    end,

    [5] = function(offset)
        gen_random_asteroids(offset)
    end,
}

--
-- Generate random world coordinate
--
function random_coord()
    local half_w = game.SCREEN_WIDTH / 2
    local half_h = game.SCREEN_HEIGHT / 2
    return {
        x = math.random(-half_w, half_w),
        y = math.random(-half_h, half_h)
    }
end

--
-- Random asteroids generator
--
function gen_random_asteroids(offset)
    local min = 6
    local max = 12
    for i = 1, math.random(min, max) do
        local coord = random_coord()
        game.add_asteroid(coord.x, offset + coord.y, 0, 0, 1.38)
    end
end

function tick()
    -- compute stage index and if changed, load the next stage
    local current_stage = start_stage + math.floor(
        (time * game.SCROLL_SPEED) / game.SCREEN_HEIGHT)
    if stage ~= current_stage then
        -- load current stage on first tick
        if stage == nil then
            level[current_stage](0)
        end

        -- pre-load next stage with an offset
        stage = current_stage
        local stage_factory = level[stage + 1]
        if stage_factory then
            stage_factory(-game.SCREEN_HEIGHT)
        end

        print("Stage", stage)
    end

    -- advance time
    time = time + 1
end