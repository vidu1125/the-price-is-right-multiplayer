import { requestHistory } from "../services/gameSocket";

export function useHistory() {
  const viewHistory = () => {
    requestHistory();
  };

  return { viewHistory };
}
