#define BIORBD_API_EXPORTS
#include "Muscles/Muscles.h"

#include "Utils/Error.h"
#include "RigidBody/GeneralizedCoordinates.h"
#include "RigidBody/GeneralizedTorque.h"
#include "Muscles/Muscle.h"
#include "Muscles/MuscleGroup.h"
#include "Muscles/StateDynamics.h"
#include "Muscles/Force.h"

biorbd::muscles::Muscles::Muscles(){

}

biorbd::muscles::Muscles::~Muscles(){

}


void biorbd::muscles::Muscles::addMuscleGroup(
        const biorbd::utils::String &name,
        const biorbd::utils::String &originName,
        const biorbd::utils::String &insertionName){
    if (m_mus.size() > 0)
        biorbd::utils::Error::error(getGroupId(name)==-1, "Muscle group already defined");

    m_mus.push_back(biorbd::muscles::MuscleGroup(name, originName, insertionName));
}

int biorbd::muscles::Muscles::getGroupId(const biorbd::utils::String &name) const{
    for (unsigned int i=0; i<m_mus.size(); ++i)
        if (!name.compare(m_mus[i].name()))
            return static_cast<int>(i);
    return -1;
}

biorbd::muscles::MuscleGroup &biorbd::muscles::Muscles::muscleGroup_nonConst(unsigned int idx)
{
    biorbd::utils::Error::error(idx<nbMuscleGroups(), "Idx asked is higher than number of muscle groups");
    return m_mus[idx];
}

const biorbd::muscles::MuscleGroup &biorbd::muscles::Muscles::muscleGroup(unsigned int idx) const{
    biorbd::utils::Error::error(idx<nbMuscleGroups(), "Idx asked is higher than number of muscle groups");
    return m_mus[idx];
}
const biorbd::muscles::MuscleGroup &biorbd::muscles::Muscles::muscleGroup(const biorbd::utils::String& name) const{
    int idx = getGroupId(name);
    biorbd::utils::Error::error(idx!=-1, "Group name could not be found");
    return muscleGroup(static_cast<unsigned int>(idx));
}

// From muscle activation (return muscle force)
biorbd::rigidbody::GeneralizedTorque biorbd::muscles::Muscles::muscularJointTorque(
        biorbd::rigidbody::Joints& m,
        const std::vector<biorbd::muscles::StateDynamics> &state,
        Eigen::VectorXd & F,
        bool updateKin,
        const biorbd::rigidbody::GeneralizedCoordinates* Q,
        const biorbd::rigidbody::GeneralizedCoordinates* QDot){

    // Update de la position musculaire
    if (updateKin > 0)
        updateMuscles(m,*Q,*QDot,updateKin);

    std::vector<std::vector<std::shared_ptr<biorbd::muscles::Force>>> force_tp = musclesForces(m, state, false);
    F = Eigen::VectorXd::Zero(static_cast<unsigned int>(force_tp.size()));
    for (unsigned int i=0; i<force_tp.size(); ++i)
        F(i) = (force_tp[i])[0]->norme();

    return muscularJointTorque(m, F, false, Q, QDot);
}

// From muscle activation (do not return muscle force)
biorbd::rigidbody::GeneralizedTorque biorbd::muscles::Muscles::muscularJointTorque(
        biorbd::rigidbody::Joints& m,
        const std::vector<biorbd::muscles::StateDynamics>& state,
        bool updateKin,
        const biorbd::rigidbody::GeneralizedCoordinates* Q,
        const biorbd::rigidbody::GeneralizedCoordinates* QDot){
    biorbd::rigidbody::GeneralizedCoordinates dummy;
    return muscularJointTorque(m, state, dummy, updateKin, Q, QDot);
}

// From Muscular Force
biorbd::rigidbody::GeneralizedTorque biorbd::muscles::Muscles::muscularJointTorque(
        biorbd::rigidbody::Joints& m,
        const Eigen::VectorXd& F,
        bool updateKin,
        const biorbd::rigidbody::GeneralizedCoordinates* Q,
        const biorbd::rigidbody::GeneralizedCoordinates* QDot){

    // Update de la position musculaire
    if (updateKin > 0)
        updateMuscles(m,*Q,*QDot,updateKin);

    // Récupérer la matrice jacobienne et
    // récupérer les forces de chaque muscles
    biorbd::utils::Matrix jaco(musclesLengthJacobian(m));

    // Calcul de la réaction des forces sur les corps
    return biorbd::rigidbody::GeneralizedTorque(-jaco.transpose() * F);
}

std::vector<std::vector<std::shared_ptr<biorbd::muscles::Force>>> biorbd::muscles::Muscles::musclesForces(
        biorbd::rigidbody::Joints& m,
        const std::vector<biorbd::muscles::StateDynamics> &state,
        bool updateKin,
        const biorbd::rigidbody::GeneralizedCoordinates* Q,
        const biorbd::rigidbody::GeneralizedCoordinates* QDot){

    // Update de la position musculaire
    if (updateKin > 0)
        updateMuscles(m,*Q,*QDot,updateKin);

    // Variable de sortie
    std::vector<std::vector<std::shared_ptr<biorbd::muscles::Force>>> forces; // Tous les muscles/Deux pointeurs par muscles (origine/insertion)

    unsigned int cmpMus(0);
    std::vector<biorbd::muscles::MuscleGroup>::iterator grp=m_mus.begin();
    for (unsigned int i=0; i<m_mus.size(); ++i) // groupe musculaire
        for (unsigned int j=0; j<(*(grp+i)).nbMuscles(); ++j){
            // forces musculaire
            forces.push_back((*(grp+i)).muscle(j)->force(*(state.begin()+cmpMus)));
            cmpMus++;
        }

    // Les forces
    return forces;
}

unsigned int biorbd::muscles::Muscles::nbMuscleGroups() const {
    return static_cast<unsigned int>(m_mus.size());
}

biorbd::utils::Matrix biorbd::muscles::Muscles::musclesLengthJacobian(biorbd::rigidbody::Joints &m)
{
    biorbd::utils::Matrix tp(biorbd::utils::Matrix::Zero(nbMuscleTotal(), m.nbDof()));
    unsigned int cmpMus(0);
    for (unsigned int i=0; i<nbMuscleGroups(); ++i){ // groupe musculaire
        for (unsigned int j=0; j<(m_mus[i]).nbMuscles(); ++j){
            // forces musculaire
            tp.block(cmpMus,0,1,m.nbDof()) = (m_mus[i]).muscle(j)->position().jacobianLength();
            ++cmpMus;
        }
    }
    return tp;

}

biorbd::utils::Matrix biorbd::muscles::Muscles::musclesLengthJacobian(
        biorbd::rigidbody::Joints& m,
        const biorbd::rigidbody::GeneralizedCoordinates &Q){

    // Update de la position musculaire
    updateMuscles(m, Q, true);
    return musclesLengthJacobian(m);
}


unsigned int biorbd::muscles::Muscles::nbMuscleTotal() const{
    unsigned int total(0);
    for (unsigned int grp=0; grp<m_mus.size(); ++grp) // groupe musculaire
        total += m_mus[grp].nbMuscles();
    return total;
}

void biorbd::muscles::Muscles::updateMuscles(
        biorbd::rigidbody::Joints& m,
        const biorbd::rigidbody::GeneralizedCoordinates& Q,
        const biorbd::rigidbody::GeneralizedCoordinates& QDot,
        bool updateKin){

    // Updater tous les muscles
    int updateKinTP;
    if (updateKin)
        updateKinTP = 2;
    else
        updateKinTP = 0;

    std::vector<biorbd::muscles::MuscleGroup>::iterator grp=m_mus.begin();
    for (unsigned int i=0; i<m_mus.size(); ++i) // groupe musculaire
        for (unsigned int j=0; j<(*(grp+i)).nbMuscles(); ++j){
            (*(grp+i)).muscle(j)->updateOrientations(m, Q, QDot, updateKinTP);
            updateKinTP=1;
        }
}
void biorbd::muscles::Muscles::updateMuscles(
        biorbd::rigidbody::Joints& m,
        const biorbd::rigidbody::GeneralizedCoordinates& Q,
        bool updateKin){

    // Updater tous les muscles
    int updateKinTP;
    if (updateKin)
        updateKinTP = 2;
    else
        updateKinTP = 0;

    // Updater tous les muscles
    std::vector<biorbd::muscles::MuscleGroup>::iterator grp=m_mus.begin();
    for (unsigned int i=0; i<m_mus.size(); ++i) // groupe musculaire
        for (unsigned int j=0; j<(*(grp+i)).nbMuscles(); ++j){
            (*(grp+i)).muscle(j)->updateOrientations(m, Q,updateKinTP);
            updateKinTP=1;
        }
}
void biorbd::muscles::Muscles::updateMuscles(
        std::vector<std::vector<biorbd::muscles::MuscleNode>>& musclePointsInGlobal,
        std::vector<biorbd::utils::Matrix> &jacoPointsInGlobal,
        const biorbd::rigidbody::GeneralizedCoordinates& QDot){
    std::vector<biorbd::muscles::MuscleGroup>::iterator grp=m_mus.begin();
    unsigned int cmpMuscle = 0;
    for (unsigned int i=0; i<m_mus.size(); ++i) // groupe musculaire
        for (unsigned int j=0; j<(*(grp+i)).nbMuscles(); ++j){
            (*(grp+i)).muscle(j)->updateOrientations(musclePointsInGlobal[cmpMuscle], jacoPointsInGlobal[cmpMuscle], QDot);
            ++cmpMuscle;
        }
}
void biorbd::muscles::Muscles::updateMuscles(
        std::vector<std::vector<biorbd::muscles::MuscleNode>>& musclePointsInGlobal,
        std::vector<biorbd::utils::Matrix> &jacoPointsInGlobal){
    // Updater tous les muscles
    std::vector<biorbd::muscles::MuscleGroup>::iterator grp=m_mus.begin();
    unsigned int cmpMuscle = 0;
    for (unsigned int i=0; i<m_mus.size(); ++i) // groupe musculaire
        for (unsigned int j=0; j<(*(grp+i)).nbMuscles(); ++j){
            (*(grp+i)).muscle(j)->updateOrientations(musclePointsInGlobal[cmpMuscle], jacoPointsInGlobal[cmpMuscle]);
            ++cmpMuscle;
        }
}