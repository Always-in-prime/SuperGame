#include "Sprite.h"
#include "../Game/CharacterModel.h"
#include "../Core/MeshRender.h"
#include <cmath>

namespace {

    // Минимальная дистанция, ближе которой противник не рисуется
    // (камера почти внутри модели).
    constexpr double kMinRenderDist = 0.30;

    // Радиус габаритной сферы персонажа (для углового отсева).
    // Должен покрывать максимальный вылет рук/ног от центра.
    constexpr double kCharBoundingRadius = 0.9;

    // Нормализация угла в [-PI, PI] без ветвлений-циклов.
    inline double WrapAngle(double a) noexcept {
        // fmod даёт [-2PI, 2PI] для |a| < ~1e15, дальше теряет точность,
        // но для игровых углов это не проблема.
        a = std::fmod(a + PI, 2.0 * PI);
        if (a < 0.0) a += 2.0 * PI;
        return a - PI;
    }

    // Проверка: попадает ли объект радиуса r в FOV камеры.
    // halfFov — половина горизонтального FOV в радианах.
    inline bool InFov(double dx, double dy, double dist2,
        double camAngle, double halfFov, double radius) noexcept
    {
        // Угловой радиус модели: atan2(r, dist).
        // При dist -> 0 объект занимает весь экран — не отсекаем.
        const double dist = std::sqrt(dist2);
        if (dist < radius) return true;

        const double angularRadius = std::asin(radius / dist); // точнее atan2(r,sqrt(d²-r²))
        const double limit = halfFov + angularRadius;

        // Угол направления: atan2(dx, dy) — как в исходнике (Y — «вперёд»).
        const double angleTo = std::atan2(dx, dy);
        const double diff = WrapAngle(angleTo - camAngle);
        return std::fabs(diff) <= limit;
    }

} // namespace

void RenderOtherPlayer(const Player& viewer, const Player& target,
    int screen_left, int view_width, int half_height, const double* zbuffer)
{
    if (target.deathTimer > 0) return;

    const double dx = target.renderX - viewer.renderX;
    const double dy = target.renderY - viewer.renderY;
    const double dist2 = dx * dx + dy * dy;

    // Отсев по минимальной дистанции (квадрат, без sqrt).
    constexpr double kMinRenderDist2 = kMinRenderDist * kMinRenderDist;
    if (dist2 < kMinRenderDist2) return;

    // Отсев по FOV с учётом углового размера модели.
    if (!InFov(dx, dy, dist2, viewer.renderAngle, FOV * 0.5,
        kCharBoundingRadius))
        return;

    // Собираем меш (в мировых координатах — так задумано API).
    Mesh mesh;
    BuildCharacterMesh(mesh, target);

    // Рендер в WorldSpace. Back-compat overload сам построит MeshXform.
    // Если хочется сэкономить — можно построить xform здесь и вызвать
    // быструю версию, но это требует ручного расчёта (см. ниже).
    RenderMeshWorldSpace(mesh,
        viewer.renderX, EYE_HEIGHT, viewer.renderY,
        viewer.renderAngle,
        screen_left, view_width, half_height,
        zbuffer);
}