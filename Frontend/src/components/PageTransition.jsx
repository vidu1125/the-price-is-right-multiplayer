import { motion } from "framer-motion";

const PageTransition = ({ children }) => {
  return (
    <motion.div
      initial={{ opacity: 0, y: 10 }} // Lúc mới vào: mờ và hơi lệch xuống
      animate={{ opacity: 1, y: 0 }}  // Lúc hiển thị: rõ nét và đúng vị trí
      exit={{ opacity: 0, y: -10 }}   // Lúc rời đi: mờ và hơi lệch lên
      transition={{ duration: 0.3 }}  // Thời gian chuyển (0.3 giây)
      style={{ width: '100%', height: '100%' }}
    >
      {children}
    </motion.div>
  );
};

export default PageTransition;