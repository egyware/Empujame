#include <Common.h>

extern unsigned int jugadores;
class Listener : public b2ContactListener{
    /// Called when two fixtures begin to touch.
    void BeginContact(b2Contact* contact) {
        b2Body *a = contact->GetFixtureA()->GetBody();
        b2Body *b = contact->GetFixtureB()->GetBody();
        Client *c_a = static_cast<Client*>(a->GetUserData());
        Client *c_b = static_cast<Client*>(b->GetUserData());
        if(c_a != NULL && c_b != NULL){ //si ambos son distintos a NULL
//            b2Vec2 v_a = a->GetLinearVelocity();
//            b2Vec2 v_b = b->GetLinearVelocity();
//            v_a.Normalize();
//            v_b.Normalize();
//
//            v_a.x = v_a.x*-50.0f;
//            v_a.y = v_a.y*-50.0f;
//
//            v_b.x = v_b.x*-50.0f;
//            v_b.y = v_b.y*-50.0f;
//            a->ApplyForceToCenter(v_a);
//            b->ApplyForceToCenter(v_b);
            printf("> Choco '%s' con '%s'\n",c_a->nick,c_b->nick);
        }
    }
    /// Called when two fixtures cease to touch.
    void EndContact(b2Contact* contact) {
        b2Body *a = contact->GetFixtureA()->GetBody();
        b2Body *b = contact->GetFixtureB()->GetBody();
        void * userData_a = a->GetUserData();
        void * userData_b = b->GetUserData();
        Client *c = static_cast<Client*>((userData_a)?userData_a:userData_b);

        if((userData_a != NULL) xor (userData_b != NULL)){ //entonces el cliente C se fue
            if(!c->caida){ //si no se ha caido
                c->caida = true;
                jugadores--;
                printf("> Cliente se cayo '%s'\n",c->nick);
            }
        }
    }

    /// This is called after a contact is updated. This allows you to inspect a
    /// contact before it goes to the solver. If you are careful, you can modify the
    /// contact manifold (e.g. disable contact).
    /// A copy of the old manifold is provided so that you can detect changes.
    /// Note: this is called only for awake bodies.
    /// Note: this is called even when the number of contact points is zero.
    /// Note: this is not called for sensors.
    /// Note: if you set the number of contact points to zero, you will not
    /// get an EndContact callback. However, you may get a BeginContact callback
    /// the next step.
    void PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
    {
        B2_NOT_USED(contact);
        B2_NOT_USED(oldManifold);
    }

    /// This lets you inspect a contact after the solver is finished. This is useful
    /// for inspecting impulses.
    /// Note: the contact manifold does not include time of impact impulses, which can be
    /// arbitrarily large if the sub-step is small. Hence the impulse is provided explicitly
    /// in a separate data structure.
    /// Note: this is only called for contacts that are touching, solid, and awake.
    void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse)
    {
        B2_NOT_USED(contact);
        B2_NOT_USED(impulse);
    }
};
