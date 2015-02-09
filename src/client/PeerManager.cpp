/* Alterado: Eliseu César Miguel
 * 13/01/2015
 * Várias alterações para comportar a separação entre In e Out
 */

#include "PeerManager.hpp"

/*somente teste... apagar esse método
void PeerManager::apagaPeerListout()
{
	peerActiveOut.clear();
}
*///***** teste teste ****************


PeerManager::PeerManager()
{
}

ServerAuxTypes PeerManager::GetPeerManagerState()
{
	return this->peerManagerState;
}

void PeerManager::SetPeerManagerState(ServerAuxTypes newPeerManagerState)
{
	cout<<"ENTRANDO"<<endl;
	cout<<" novo modo... "<<newPeerManagerState<<endl;
	cout<<"$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$"<<endl;
	boost::mutex::scoped_lock peerListLock(peerListMutex);
	boost::mutex::scoped_lock peerActiveLock(peerActiveMutexOut);
	switch (newPeerManagerState)
	{

	case SERVER_AUX_ACTIVE:
		if (this->peerManagerState == NO_SERVER_AUX) //iniciando o processo de servidor auxiliar
		{
			//isso vai virar estratégia ....
		    peerActiveOut.clear();

		    this->peerManagerState = newPeerManagerState;
		    cout<<" atuando como servidor auxiliar"<<endl;
		}
		break;

	case SERVER_AUX_MESCLAR:
		if (this->peerManagerState == SERVER_AUX_ACTIVE) //entrando em processo de mesclagem
		{

			//isso vai virar estratégia
		    //for (set<string>::iterator i = peerActiveOut.begin(); i != peerActiveOut.end(); i++)
		    //	peerList[*i].SetChannelId_Sub(0);

			int size = (int) peerActiveOut.size() / 2;
		    while (size > 0)
			{
		    	size--;
	    		peerActiveOut.erase(peerActiveOut.begin());
		    }

			this->peerManagerState = newPeerManagerState;
			cout<<" atuando como servidor de mesclagem"<<endl;
		}
		break;

	case NO_SERVER_AUX:

		if (this->peerManagerState == SERVER_AUX_MESCLAR) //fim da mesclagem
		{
		    for (set<string>::iterator i = peerActiveOut.begin(); i != peerActiveOut.end(); i++)
		    	peerList[*i].SetChannelId_Sub(0);

			this->peerManagerState = newPeerManagerState;
		}

		if (this->peerManagerState == SERVER_AUX_ACTIVE) //saindo de server sem mesclar
		{
		    for (set<string>::iterator i = peerActiveOut.begin(); i != peerActiveOut.end(); i++)
		    	peerList[*i].SetChannelId_Sub(0);

		    int size = (int) peerActiveOut.size() / 2;
		    while (size > 0)
			{
		    	size--;
	    		peerActiveOut.erase(peerActiveOut.begin());
		    }

			this->peerManagerState = newPeerManagerState;
		}
		cout<<" atuando como servidor não servidor auxiliar"<<endl;
        break;

    default:
        cout<<"Não houve mudança no Estado do PeerManager"<<endl;
        break;
	}
	cout<<"novo estado do manager "<<peerManagerState<<endl;
	peerActiveLock.unlock();
	peerListLock.unlock();
}

unsigned int PeerManager::GetMaxActivePeers(set<string>* peerActive)
{
	if (peerActive == &peerActiveIn) return maxActivePeersIn;
	if (peerActive == &peerActiveOut) return maxActivePeersOut;
	return 0;
}

void PeerManager::SetMaxActivePeersIn(unsigned int maxActivePeers)
{
	// se a estratégia de mesclagem permitir mudar o tamanho da lista, usar o mutex
	//boost::mutex::scoped_lock peerActiveLock(peerActiveMutexOut);
	this->maxActivePeersIn = maxActivePeers;
	//peerActiveLock.unlock();
}
void PeerManager::SetMaxActivePeersOut(unsigned int maxActivePeers)
{
	// se a estratégia de mesclagem permitir mudar o tamanho da lista, usar o mutex
	//boost::mutex::scoped_lock peerActiveLock(peerActiveMutexOut);
	this->maxActivePeersOut = maxActivePeers;
	//peerActiveLock.unlock();
}

bool PeerManager::AddPeer(Peer* newPeer)
{
	boost::mutex::scoped_lock peerListLock(peerListMutex);
	if (peerList.find(newPeer->GetID()) == peerList.end())
	{
		peerList[newPeer->GetID()] = PeerData(newPeer);
		if (peerManagerState == SERVER_AUX_ACTIVE)
			peerList[newPeer->GetID()].SetChannelId_Sub(1);
		peerListLock.unlock();
		cout<<"Peer "<<newPeer->GetID()<<" added to PeerList"<<endl;
		return true;
	}
	peerListLock.unlock();
	return false;
}

set<string>* PeerManager::GetPeerActiveIn()
{
	return &peerActiveIn;
}
set<string>* PeerManager::GetPeerActiveOut()
{
	return &peerActiveOut;
}

map<string, unsigned int>* PeerManager::GetPeerActiveCooldown(set<string>* peerActive)
{
	if (peerActive == &peerActiveIn) return &peerActiveCooldownIn;
	if (peerActive == &peerActiveOut) return &peerActiveCooldownOut;
	return NULL;
}

//ECM - efetivamente, insere o par em uma das lista In ou Out
//neste método, é certo que o par pertence a peerList...
bool PeerManager::ConnectPeer(string peer, set<string>* peerActive)
{
	boost::mutex* peerActiveMutex = this->GetPeerActiveMutex(peerActive);
	map<string, unsigned int>* peerActiveCooldown = this->GetPeerActiveCooldown(peerActive);
	if (peerActiveCooldown->find(peer) == (*peerActiveCooldown).end())
	{
		boost::mutex::scoped_lock peerActiveLock(*peerActiveMutex);
		if (peerActive->size() < this->GetMaxActivePeers(peerActive))
		{

			/* controle de servidor auxiliar
			 * aceita somente pares da rede paralela
			 */
			if (peerManagerState == SERVER_AUX_ACTIVE)
				if (peerList[peer].GetChannelId_Sub() != 1)
					return false;

			/* controle de servidor auxiliar
			* durante a mesclagem não aceita um par da rede
			* paralalela que ele removeu ser out...
			*/
			if (peerManagerState == SERVER_AUX_MESCLAR)
				if (peerList[peer].GetChannelId_Sub() == 1)
					return false;

			if (peerActive->insert(peer).second)
			{
				peerActiveLock.unlock();
				string list;
				if (*(peerActive) == peerActiveIn)
					list = "In";
				else
					 list = "out";
				cout<<"Peer "<<peer<<" connected to PeerActive "<<list<<endl;
				return true;
			}
		}
		peerActiveLock.unlock();
	}
	return false;
}

void PeerManager::DisconnectPeer(string peer, set<string>* peerActive)
{
	boost::mutex* peerActiveMutex = this->GetPeerActiveMutex(peerActive);
	map<string, unsigned int>* peerActiveCooldown = this->GetPeerActiveCooldown(peerActive);
	boost::mutex::scoped_lock peerActiveLock(*peerActiveMutex);
	peerActive->erase(peer);
	peerActiveLock.unlock();
	(*peerActiveCooldown)[peer] = PEER_ACTIVE_COOLDOWN;
    cout<<"Peer "<<peer<<" disconnected from PeerActive"<<endl;
}

void PeerManager::RemovePeer(string peer)
{
	boost::mutex::scoped_lock peerListLock(peerListMutex);
	if (peerManagerState == SERVER_AUX_ACTIVE && peerList[peer].GetChannelId_Sub() == 0)
		return;
	peerList.erase(peer);
	peerListLock.unlock();
	cout<<"Peer "<<peer<<" removed from PeerList"<<endl;
}

unsigned int PeerManager::GetPeerActiveSize(set<string>* peerActive)
{
	boost::mutex* peerActiveMutex = this->GetPeerActiveMutex(peerActive);
	boost::mutex::scoped_lock peerActiveLock(*peerActiveMutex);
	unsigned int size = peerActive->size();
	peerActiveLock.unlock();
	return size;
}

// Gera o total de parceiros somando In e Out sem repeticoes
unsigned int PeerManager::GetPeerActiveSizeTotal()
{
	unsigned int size = this->GetPeerActiveSize(&peerActiveIn);
	boost::mutex::scoped_lock peerActiveInLock(peerActiveMutexIn);
	boost::mutex::scoped_lock peerActiveOutLock(peerActiveMutexOut);
	for (set<string>::iterator i = peerActiveOut.begin(); i != peerActiveOut.end(); i++)
	{
		if (peerActiveIn.find(*i) == peerActiveIn.end())
			size++;
	}
	peerActiveInLock.unlock();
	peerActiveOutLock.unlock();
    return size;
}

bool PeerManager::IsPeerActive(string peer,set<string>* peerActive)
{
	boost::mutex* peerActiveMutex = this->GetPeerActiveMutex(peerActive);
	boost::mutex::scoped_lock peerActiveLock(*peerActiveMutex);
	if (peerActive->find(peer) != peerActive->end())
	{
		peerActiveLock.unlock();
		return true;
	}
	peerActiveLock.unlock();
	return false;
}


PeerData* PeerManager::GetPeerData(string peer)
{
	return &peerList[peer];
}


map<string, PeerData>* PeerManager::GetPeerList()
{
	//mutex travado pelo usuário: Classe Connector...
	if (peerManagerState == SERVER_AUX_ACTIVE)
	{
	    for (map<string, PeerData>::iterator i = peerList.begin(); i != peerList.end(); i++)
	    	if (i->second.GetChannelId_Sub() == 1)
	    		peerListMasterChannel[i->first] = i->second;
	    //cout<<"enviando lista auliliar. Modo = "<<peerManagerState<<endl;
	    return &peerListMasterChannel;
	}
	//cout<<"enviando lista total ao connector"<<endl;
	return &peerList;
}
boost::mutex* PeerManager::GetPeerListMutex()
{
	return &peerListMutex;
}

boost::mutex* PeerManager::GetPeerActiveMutex(set<string>* peerActive)
{
	if (peerActive == &peerActiveIn) return &peerActiveMutexIn;
	if (peerActive == &peerActiveOut) return &peerActiveMutexOut;
	return NULL;
}

//ECM metodo privado criado para ser chamado duas vezes (In e Out) em CheckPeerList()
void PeerManager::CheckpeerActiveCooldown(map<string, unsigned int>* peerActiveCooldown)
{
	set<string> deletedPeer;
	for (map<string, unsigned int>::iterator i = peerActiveCooldown->begin(); i != peerActiveCooldown->end(); i++)
	   {
		i->second--;
		if (i->second == 0)
			deletedPeer.insert(i->first);
	}
	for (set<string>::iterator i = deletedPeer.begin(); i != deletedPeer.end(); i++)
    {
	       peerActiveCooldown->erase(*i);
	}
	deletedPeer.clear();
}

void PeerManager::CheckPeerList()
{
	//ECM Tabela de decisao para remover peer.
	//|----------------------------------------------------------------------------------------------------------|
	//| ttlIn ttlOut PeerActiveIn    PeerActiveOut |  Desconectar In | Desconectar Out | Remover PeerList | caso |
	//|----------------------------------------------------------------------------------------------------------|
	//|   0    <>0     pertence        pertence    |       X         |                 |                  |   1  |
	//|  <>0    0      pertence        pertence    |                 |        X        |                  |   2  |
	//|   0     0      pertence        pertence    |       X         |        X        |     X            |   3  |
	//|   0    <>0     pertence      nao pertence  |       X         |                 |     X            |   4  |
	//|  <>0    0    nao pertence      pertence    |                 |        X        |     X            |   5  |
    //|----------------------------------------------------------------------------------------------------------|

    set<string> desconectaPeerIn;  //DesconectarIn
    set<string> desconectaPeerOut; //DesconectarOut
    set<string> deletaPeer;        //Remover

    bool isPeerActiveIn = false;
    bool isPeerActiveOut  = false;

    boost::mutex::scoped_lock peerActiveInLock(peerActiveMutexIn);
    boost::mutex::scoped_lock peerActiveOUTLock(peerActiveMutexOut);

    //gera lista com todos os pares ativos
    set<string> temp_allActivePeer (peerActiveIn);
    for (set<string>::iterator i = peerActiveOut.begin(); i != peerActiveOut.end(); i++)
    	temp_allActivePeer.insert(*i);

    for (set<string>::iterator i = temp_allActivePeer.begin(); i != temp_allActivePeer.end(); i++)
    {
    	isPeerActiveIn = peerActiveIn.find(*i) != peerActiveIn.end();
    	if (isPeerActiveIn)
    	{
    		peerList[*i].DecTTLIn();
    		if (peerList[*i].GetTTLIn() <= 0)
    		{
    			desconectaPeerIn.insert(*i);
    			isPeerActiveIn = false;
    		}
    	}
    	isPeerActiveOut = peerActiveOut.find(*i) != peerActiveOut.end();
    	if (isPeerActiveOut)
    	{
    		peerList[*i].DecTTLOut();
    		if (peerList[*i].GetTTLOut() <= 0)
    	    {
    			desconectaPeerOut.insert(*i);
    			isPeerActiveOut = false;
    		}

    	}
    	if ((!isPeerActiveIn) && (!isPeerActiveOut))
     		deletaPeer.insert(*i);
    }
    peerActiveInLock.unlock();
	peerActiveOUTLock.unlock();

    for (set<string>::iterator i = desconectaPeerIn.begin(); i != desconectaPeerIn.end(); i++)
    {
    	DisconnectPeer(*i, &peerActiveIn);
    }
    for (set<string>::iterator i = desconectaPeerOut.begin(); i != desconectaPeerOut.end(); i++)
        {
        	DisconnectPeer(*i, &peerActiveOut);
        }
    for (set<string>::iterator i = deletaPeer.begin(); i != deletaPeer.end(); i++)
    {
        RemovePeer(*i);
    }

    this->CheckpeerActiveCooldown(&peerActiveCooldownIn);
    this->CheckpeerActiveCooldown(&peerActiveCooldownOut);
}

//funcao auxiliar usada interna em ShowPeerList para impressao
int PeerManager::showPeerActive(set<string>* peerActive)
{
	boost::mutex* peerActiveMutex = this->GetPeerActiveMutex(peerActive);
	int j = 0; int ttl = 0; string ttlRotulo("");
	boost::mutex::scoped_lock peerActiveLock(*peerActiveMutex);

    for (set<string>::iterator i = (*peerActive).begin(); i != (*peerActive).end(); i++, j++)
	{
    	if (peerActive == &peerActiveIn)
        {
        	ttl = peerList[*i].GetTTLIn();
        	ttlRotulo == "TTLIn";
        }
        else
        {
        	ttl = peerList[*i].GetTTLOut();
        	ttlRotulo == "TTLOut";
        }

	    cout<<"Key: "<<*i<<" ID: "<<peerList[*i].GetPeer()->GetID()<<" Mode: "<<(int)peerList[*i].GetMode()<<ttlRotulo<<": "<<ttl << " PR: "<<peerList[*i].GetPendingRequests() << endl;
	}
	peerActiveLock.unlock();
	return j;

}

void PeerManager::ShowPeerList()
{
 	int k = 0;
    int j = 0;
    cout<<endl<<"- Peer List Active -"<<endl;
    k = showPeerActive(&peerActiveIn);
	//cout<<"Total: "<<k<<" Peers In"<<endl<<endl;
    j = showPeerActive(&peerActiveOut);
    cout<<"Total In ["<<k<<"]  Total Out ["<<j<<"]"<<endl;
    cout<<"Total different Peers Active: ["<<this->GetPeerActiveSizeTotal()<<"] "<<endl<<endl;

	j = 0;
	cout<<endl<<"- Peer List Total -"<<endl;
	boost::mutex::scoped_lock peerListLock(peerListMutex);
    for (map<string, PeerData>::iterator i = peerList.begin(); i != peerList.end(); i++, j++)
	{
		cout<<"Key: "<<i->first<< endl;
		cout<<"ID: "<<i->second.GetPeer()->GetID()<<" Mode: "<<(int)i->second.GetMode()<<" TTLIn: "<<i->second.GetTTLIn() <<" TTLOut: "<<i->second.GetTTLOut() << " RTT(delay): " <<i->second.GetDelay()<< "s PR: "<<i->second.GetPendingRequests() << endl;
	}
	peerListLock.unlock();
	cout<<"Total PeerList: "<<j<<endl<<endl;

}

