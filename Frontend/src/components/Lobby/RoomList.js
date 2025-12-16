// src/components/Lobby/RoomList.js
import "./RoomList.css";

const ROOMS = [
  { id: 1, name: "The Price is Right (A)", status: "Waiting" },
  { id: 2, name: "Challenge: High Stakes (B)", status: "In Game" },
  { id: 3, name: "Casual Fun Room (C)", status: "Waiting" },
  { id: 4, name: "Vietnamese Pricing Game (D)", status: "Waiting" },
  { id: 5, name: "Max Bid Only (E)", status: "Waiting" },
  // Thêm dữ liệu giả lập còn lại để kích hoạt scroll
  { id: 6, name: "Quick Play - Open Slot (F)", status: "Waiting" },
  { id: 7, name: "The Big Reveal (G)", status: "In Game" },
  { id: 8, name: "Midnight Madness (H)", status: "Waiting" },
  { id: 9, name: "Test Room 1 (I)", status: "Waiting" },
  { id: 10, name: "Global Server East (J)", status: "Waiting" },
];

export default function RoomList() {
  // Giới hạn hiển thị 5 phòng và thêm 5 phòng giả lập còn lại
  // để đảm bảo thanh cuộn luôn hiển thị (tổng cộng 10 mục)
  return (
    <table className="room-table">
      <thead>
        <tr>
          <th>ID</th>
          <th>Name</th>
          <th>Status</th>
          <th></th>
        </tr>
      </thead>
      <tbody>
        {/* CHỈNH SỬA: SỬ DỤNG map TRÊN TOÀN BỘ ROOMS (10 mục) để kích hoạt scroll */}
        {ROOMS.map((r) => (
          <tr key={r.id}>
            <td>{r.id}</td>
            <td>{r.name}</td>
            <td>{r.status}</td>
            <td>
              <button 
                onClick={() => console.log("Join", r.id)}
                disabled={r.status === "In Game"} /* Không cho join nếu đang trong game */
              >
                {r.status === "In Game" ? "Locked" : "Join"}
              </button>
            </td>
          </tr>
        ))}
      </tbody>
    </table>
  );
}